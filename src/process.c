/* process.c - Processes operations for qADSL
 */

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "process.h"
#include "daemon.h"
#include "http.h"
#include "lock.h"
#include "log.h"


int 
process (config_data_t *config, op_t operation, int verbose)
{
  int   result  = 0;
  pid_t running = lock_read (&config->pid_file);
  
  switch (operation)
    {
      /* Login, and perhaps start the autologin daemon... */
    case LOGIN:
      if (running)
        {
	 /* Force the daemon to wake up earlier and relogin. */
	 result = kill (running, SIGHUP);
	 if (result)
	   {
	     perror (PACKAGE_NAME ": Kicked but missed the login daemon.\n" PACKAGE_NAME ": Maybe a stale lockfile is present and the damon is not running?\n" PACKAGE_NAME ": Or maybe the process is running as root, but you are not?\n" PACKAGE_NAME);
	     return EXIT_FAILURE;
	   }
	 else
	   {
	     return EXIT_SUCCESS;
	   }
        }

      result = http_pre_login (config, verbose);
      if (!result)
        {
          /* Test if we're logged in already. */
          if (!strstr (config->get_msg, config->logged_in_string))
            {
              /* Nope, login first. */
              result = http_internet_login (config, verbose);
            }
          
          if (!result)
            {
              config->logged_in = 1;
            }
        }
      if (config->daemon_start)
        {
          if (lock_remove (config->pid_file))
            {
	      /* Non-fatal error, it's OK if we run the first time. */
              if (EACCES == errno)
                {
                  fprintf (stderr, "%s: Failed to delete old PID file, %s - %s\n", 
			   PACKAGE_NAME, config->pid_file, strerror (errno));
                }
            }
          
          /* Always try to fork off the daemon. */
          if (daemonize () == 0)
            {
              daemon_thread (config, verbose ? LOG_FILE : LOG_NONE);
            }
        }
      break;

      /* Logout, discarding all possible errors - just logout */
    case LOGOUT:
      if (!running)
        {
          fprintf (stderr, "No active session found, will logout anyway.\n");
        }
      else
        {
	  /* Send SIGTERM to gracefully let the daemon process .. exit. */
	  kill (running, SIGTERM);
          lock_remove (config->pid_file);
        }
      result = http_internet_logout (config, verbose);
      break;

    case NOP:
      printf ("Neither login or logout selected. Reverting to query status.\n");
    default:
    case STATUS:
      if (running)
        {
          printf ("%s running with PID = %d\n", PACKAGE_NAME, running);
        } 
      else
        {
          printf ("%s not running.\n", PACKAGE_NAME);
        }

      result = 0;
    }

  return result;
}
