/* process.c - Processes operations for qADSL
 */

#include <errno.h>
#include <stdio.h>
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
  int result, tries;
  pid_t running = lock_read (PID_FILE);
  
  switch (operation)
    {
    case LOGIN:
      if (running)
	{
	  fprintf (stderr, 
		   "qADSL already running on pid %d %s", running,
		   "- logout the current session first.\n");
	  return EXIT_FAILURE;
	}
      break;

    case LOGOUT:
      if (!running)
	{
	  fprintf (stderr, "No active session found, will logout anyway.\n");
	}
      else
	{
	  daemon_kill (running);
	  lock_remove (PID_FILE);
	}
      break;

    case STATUS:
      if (running)
	{
	  printf ("%s running with PID = %d\n", PACKAGE_NAME, running);
	}
      else
	{
	  printf ("%s not running.\n", PACKAGE_NAME);
	}
    case NOP:
    default:
      return 0;
    }

  /* Login, and perhaps start the autologin daemon... */
  if (operation == LOGIN)
    {
      result = pre_login (config, verbose);
      if (!result)
	result = internet_login (config, verbose);

      if (!result && config->daemon_start)
	{
	  result = lock_remove (config->pid_file);
	  if (result)
	    {
	      if (EACCES == errno)
		{
		  ERROR("%s: Failed to delete old PID file, %s - %s\n", 
			PACKAGE_NAME, config->pid_file, strerror (errno));
		}
	    }
	  else
	    {
	      /* Fork off the daemon */
	      if (daemonize () == 0)
		{
		  daemon_thread (config);
		}
	    }
	}
    }
  /* Logout, discard all errors - just logout */
  else
    {
      result = internet_logout (config, verbose);
    }

  return result;
}
