/* log.h - Handles all communication with the outside world.
 */
#ifndef __LOG_H__
#define __LOG_H__

#include <syslog.h>

#define MSG_NONE     0
#define MSG_LOG      1
#define MSG_FILE     2
#define MSG_DEBUG    4

#define IS_DEBUG(v) (MSG_DEBUG & (v))

void write_message (int level, char *fmt, ...);
void write_logfile (int level, char *fmt, ...);

#define DEBUG(args...)                             \
  if (MSG_DEBUG & verbose)                         \
    {                                              \
      if (MSG_FILE & verbose)                      \
        write_logfile (LOG_DEBUG, args);           \
      else                                         \
        write_message (LOG_DEBUG, args);           \
    }                                              \
  else                                             \
    {}

#define LOG(args...)                               \
  if (MSG_LOG & verbose)                           \
    {                                              \
      if (MSG_FILE & verbose)                      \
        write_logfile (LOG_INFO, args);            \
      else                                         \
        write_message (LOG_INFO, args);            \
    }                                              \
  else                                             \
    {}

/* We always want the errors to creep up, regardless
 * of the verbosity level. Only figure out if it
 * goes to the logfile or stderr.
 */
#define ERROR(args...)                             \
  if (MSG_FILE == verbose)                         \
    write_logfile (LOG_ERR, args);                 \
  else                                             \
    write_message (LOG_ERR, args);

#endif /* __MSG_H__ */
