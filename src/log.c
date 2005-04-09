/* log.c - Handles all communication with the outside world.
 *
 * Copyright (C) 2003, 2004 Joachim Nilsson <joachim!nilsson()member!fsf!org>
 *
 * GNU Alive is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * GNU Alive is distributed in the hope that it will be useful, but
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

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

int verbose = 0;

/* Just an empty string that is not on the stack */
static char *empty_string = "";


void write_message (int level, char *fmt, ...)
{
  time_t current_time;
  char *time_string;
  int mypid;
  va_list  ap;

  mypid = getpid ();
  time (&current_time);
  time_string = ctime (&current_time);

  if (!time_string)
    time_string = empty_string;
  else
    time_string [strlen(time_string) - 1] = 0;

  /* XXX - Temporarily removing the timestamp on console stuff. */
  time_string = empty_string;   /* XXX */

  va_start (ap, fmt);
  switch (level)
    {
    case LOG_DEBUG:
      printf (_("%s DBG %s[%d]: "), time_string, PACKAGE_NAME, mypid);
      vprintf (fmt, ap);
      printf ("\n");
      break;

    case LOG_INFO:
      printf (_("%s INF %s[%d]: "), time_string, PACKAGE_NAME, mypid);
      vprintf (fmt, ap);
      printf ("\n");
      break;

    case LOG_ERR:
    default:
      fprintf (stderr, _("%s ERR %s[%d]: "), time_string, PACKAGE_NAME, mypid);
      vfprintf (stderr, fmt, ap);
      fprintf (stderr, "\n");
      break;
    }
  va_end (ap);
}


/**
 * write_logfile: Write a log message to the syslog.
 * @level:        Importance of message,
 * @fmt:          Log message.
 *
 * This function is used solely by the login daemon to communicate
 * status, error conditions and for debugging when setting up or
 * when problems arise.
 *
 * The @level can be any of [LOG_ERR, LOG_INFO, LOG_DEBUG].
 *
 * Note: This function is wrapped by macros in <log.h>
 */

void
write_logfile (int level, char *fmt, ...)
{
  int     len;
  char   *str;
  va_list ap;

  str = alloca (MAXDATASIZE);  /* XXX - What is this, a hardcoded value?! */
  va_start (ap, fmt);
  len = vsnprintf (str, MAXDATASIZE, fmt, ap);
  if (len >= 0)
    str [len] = 0;
  else
    *str = 0;
  va_end (ap);

  openlog (PACKAGE_NAME, LOG_PID | LOG_CONS, LOG_DAEMON);
  syslog (level, str);
  closelog ();
}

/* Local Variables:
 * mode: C;
 * c-file-style: gnu;
 * indent-tabs-mode: nil;
 * End:
 */
