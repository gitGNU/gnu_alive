/* process.c - Processes operations for qADSL.
 *
 * Copyright (c) 2003,2004 Joachim Nilsson <joachim!nilsson()member!fsf!org>
 *
 * qADSL is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * qADSL is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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
          if (http_test_if_logged_in (config))
            {
              /* Nope, login first. */
              result = http_internet_login (config, verbose);
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
