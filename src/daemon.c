/* daemon.c - Holds the auto-login daemon and its code.
 *
 * Copyright (c) 2001 Jakob "kuba" Stasilowicz <kuba()unix!se>
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
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "conf.h"
#include "lock.h"
#include "log.h"
#include "http.h"


/**
 * daemonize: Fork off the deamon process and detach from session context.
 *
 * Disassociate from process group and controlling terminal forking off
 * the daemon process. The function returns a fork() pid value as result.
 *
 * Returns: A pid value. Negative value is error, positive is child pid
 *          zero indicates that the child is executing.
 */

int
daemonize (void)
{
  int fd;
  pid_t pid;

  /* Fork off daemon process */
  pid = fork ();
  if (0 != pid)
    {
      return pid;
    }

  /* or setsid() to lose control terminal and change process group */
  setpgid(0, 0);

  /* Prevent reacquisition of a controlling terminal */
  pid = fork ();
  if (0 != pid)
    {
      /* Reap the child and let the grandson live on. */
      exit (EXIT_SUCCESS);
    }

  /* If parent is NOT init. */
  if (1 != getppid ())
    {
#ifdef SIGTTOU
      /* Ignore if background tty attempts write. */
      signal (SIGTTOU, SIG_IGN);
#endif
#ifdef SIGTTIN
      /* Ignore if background tty attempts read. */
      signal (SIGTTIN, SIG_IGN);
#endif

#ifdef SIGTSTP
      /* Ignore any keyboard generated stop signal signals. */
      signal (SIGTSTP, SIG_IGN);
#endif
      /* Become session leader and group process leader
       * with no controlling terminal
       */
      setsid ();
    }

  /* Close all open files */
  for (fd = 0; fd < sysconf(_SC_OPEN_MAX); fd++)  /* XXX - Add configure check */
    {
      close (fd);
    }

  /* Move current directory off mounted file system */
  chdir ("/");
  /* Clear any inherited file mode creation mask */
  umask (0);

  return pid;
}


static void
daemon_sighandler (int signal)
{
  /* We quit on everything but HUP */
  if (SIGHUP != signal)
    {
      write_logfile (LOG_INFO, "Got signal(%d), quitting.", signal);

      exit (0);
    }
  else
    {
      write_logfile (LOG_INFO, "Got signal(%d), forcing relogin.", signal);
    }
}



/**
 * autologin_daemon - This is what keeps us going
 *
 * This thread function is the one responsible for keeping the
 * connection open.  This function is called when a properly
 * daemonized thread is started.
 *
 * XXX - Add nifty feature to log "Periodic status report: qadsl is CONNECTED"
 * and "NOT CONNECTED".  This message should be logged every 60 minutes and
 * could easily be made up from a multiple of the DELAY.
 */

void
daemon_thread (config_data_t *config, int verbose)
{
  int result, slept, latched_logged_in = -1;
  unsigned int timeout = 60 * config->daemon_delay;
  pid_t mypid = getpid ();

  signal (SIGTERM, daemon_sighandler);
  signal (SIGHUP, daemon_sighandler);

  result = lock_create (&config->pid_file, mypid);
  if (result)
    {
      /* If we cannot create the logfile we do not allow the
       * daemon to start since there is no way (other than ps)
       * to communicate the PID to the outside world.
       */
      ERROR ("Cannot write PID(%d) to file, %s: %s",
             (int)mypid, config->pid_file, strerror (errno));
      ERROR ("Aborting daemon - cannot communicate PID to outside world.");

      /* Bye bye */
      return;
    }
  else
    {
      write_logfile (LOG_INFO, "Keep-alive daemon started, pid: %d", mypid);
    }

  while (1)
    {
      if (latched_logged_in != config->logged_in)
        {
          write_logfile (LOG_INFO, "Login %s", config->logged_in ? "successful." : "FAILED!");
        }
      latched_logged_in = config->logged_in;

      /* Now, sleep before we reconnect and check status. */
      slept = sleep (timeout);

      /* The "ping" daemon only reads /sd/init */
      http_pre_login (config, verbose);

      /* The login daemon also tries to login, but only if
       * the call to http_pre_login() did *not* bring up
       * the "redirect page".
       */
      if (config->daemon_type && !config->logged_in)
        {
          LOG ("Daemon logging in again.");
          http_do_login (config, verbose);
        }

      /* This stuff added for verbosity */
      if (slept > 0)            /* Awoken by SIGHUP */
        {
          if (config->logged_in)
            {
              write_logfile (LOG_INFO, "Forced relogin successful.");
            }
          else
            {
              write_logfile (LOG_INFO, "Forced relogin FAILED!");
            }
        }
      else
        {
          LOG ("Periodic relogin %s.", config->logged_in ? "OK" : "FAILED");
        }
    }
}

/* Local Variables:
 * mode: C;
 * c-file-style: gnu;
 * indent-tabs-mode: nil;
 * End:
 */
