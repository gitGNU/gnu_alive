/* process.c - Processes operations for qADSL
 */

#include <errno.h>
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
          fprintf (stderr, 
                   "qADSL already running on pid %d - logout the current "
		   "session first.\n",
		   running);
          return EXIT_FAILURE;
        }

      result = pre_login (config, verbose);
      if (!result)
        {
          /* Test if we're logged in already. */
          if (!strstr (config->get_msg, config->logged_in_string))
            {
              /* Nope, login first. */
              result = internet_login (config, verbose);
            }
          
          if (!result)
            {
              config->logged_in = 1;
            }
        }
      if (config->daemon_start)
        {
          result = lock_remove (config->pid_file);
          if (result)
            {
              if (EACCES == errno)
                {
                  fprintf (stderr, "%s: Failed to delete old PID file, %s - %s\n", 
			   PACKAGE_NAME, config->pid_file, strerror (errno));
                }
            }
          
          /* Always try to fork off the daemon. */
          if (daemonize () == 0)
            {
              daemon_thread (config);
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
          daemon_kill (running);
          lock_remove (config->pid_file);
        }
      result = internet_logout (config, verbose);
      break;

    default:
    case NOP:
    case STATUS:
      if (running)
        {
          printf ("%s running with PID = %d\n", PACKAGE_NAME, running);
        } 
      else
        {
          printf ("%s not running.\n", PACKAGE_NAME);
        }
    }

  return result;
}
