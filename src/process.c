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
          LOG ("Already running on pid %d, forcing relogin.", running);
         result = kill (running, SIGHUP);
         if (result)
           {
             ERROR ("kill(%d, SIGHUP) failed: %s\n", running, strerror(errno));
             ERROR ("Maybe a stale lockfile (%s)?", config->pid_file);

             LOG ("Trying to remove possibly stale lockfile (%s)...", config->pid_file);
             result = lock_remove (config->pid_file);
             if (result)
               {
                 ERROR ("Couldn't remove possible stale lockfile (%s).\n",
                        config->pid_file);
                 ERROR ("Maybe the process (%d) is running as root, but you are not?",
                        running);

                 return EXIT_FAILURE;
               }
           }
         else
           {
             return EXIT_SUCCESS;
           }
        }

      /* Bring up init page - could be we're already logged in. */
      http_pre_login (config, verbose);
      if (!config->logged_in)
        {
          /* Nope, login first. */
          LOG ("First check, not logged in yet.");
          result = http_internet_login (config, verbose);
          if (result)
            {
              ERROR ("To diagnose, try the options --debug --verbose");
            }
          else
            {
              if (config->logged_in)
                {
                  LOG ("SUCCESSFUL LOGIN");
                }
              else
                {
                  LOG ("LOGIN FAILED - To diagnose, try the options --debug --verbose");
                }
            }
        }
      else
        {
          LOG ("Already logged in.");
          result = 0;
        }

      /* Start daemon last. */
      if (config->daemon_start)
        {
          if (lock_remove (config->pid_file))
            {
              /* Non-fatal error, it's OK if we run the first time. */
              if (EACCES == errno)
                {
                  ERROR ("Failed to delete old PID file, %s - %s",
                         config->pid_file, strerror (errno));
                }
            }

          /* Always try to fork off the daemon. */
          if (daemonize () == 0)
            {
              daemon_thread (config, verbose | MSG_FILE);
            }
        }
      break;

      /* Logout, discarding all possible errors - just logout */
    case LOGOUT:
      if (!running)
        {
          ERROR ("No active session found, will logout anyway.");
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
      LOG ("Neither login or logout selected. Reverting to query status.");
    default:
    case STATUS:
      http_pre_login (config, verbose);
      LOG ("Current status: %s", config->logged_in ? "CONNECTED" : "DISCONNECTED");

      if (running > 0)
        {
          LOG ("Login daemon running with PID = %d", running);
          result = running;
        }
      else
        {
          LOG ("Login daemon not running.");
          result = -1;
        }
      break;
    }

  return result;
}

/* Local Variables:
 * mode: C;
 * c-file-style: gnu;
 * indent-tabs-mode: nil;
 * End:
 */
