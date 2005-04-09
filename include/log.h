/* log.h - Handles all communication with the outside world.
 */
#ifndef __LOG_H__
#define __LOG_H__

#include <syslog.h>

#define LOG_NONE     0
#define LOG_FILE     8

#define IS_DEBUG() (LOG_DEBUG & verbose)

extern int verbose;

void write_message (int level, char *fmt, ...);
void write_logfile (int level, char *fmt, ...);

/* We always want the errors to creep up, regardless
 * of the verbosity level. Only figure out if it
 * goes to the logfile or stderr.
 */
#define DO_LOG(lvl, args...)                    \
  if ((LOG_DEBUG & verbose) || (LOG_ERR & lvl)) \
    {                                           \
      if (LOG_FILE & verbose)                   \
        write_logfile (lvl, args);              \
      else                                      \
        write_message (lvl, args);              \
    }

#define LOG(args...)    DO_LOG(LOG_INFO, args)
#define DEBUG(args...)  DO_LOG(LOG_DEBUG, args)
#define ERROR(args...)  DO_LOG(LOG_ERR, args)

#endif /* __MSG_H__ */
