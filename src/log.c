#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <syslog.h>

#include "config.h"


/**
 * write_log: Write a log message to the syslog.
 * @str: Log message
 */

void
write_log (char *fmt, ...)
{
  char str[80]; /* XXX - What is this, a hardcoded value?! */
  va_list ap;

  va_start (ap, fmt);
  vsnprintf (str, 80, fmt, ap);
  va_end (ap);

  openlog (PACKAGE_NAME, LOG_PID | LOG_CONS, LOG_DAEMON);
  syslog (LOG_INFO, str);
  closelog ();
}
