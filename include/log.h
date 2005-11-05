/* log.h - Handles all communication with the outside world.
 */
#ifndef __LOG_H__
#define __LOG_H__

#include <syslog.h>

#define LOG_FILE 8

extern int verbose;

void write_message (int level, char *fmt, ...);
void write_logfile (int level, char *fmt, ...);

#define DO_LOG(lvl, args...)                    \
  if (verbose >= lvl)                           \
    {                                           \
      if (LOG_FILE & verbose)                   \
        write_logfile (lvl, args);              \
      else                                      \
        write_message (lvl, args);              \
    }

#define IS_DBG()     (verbose >= LOG_DEBUG)
#define LOG(args...)  DO_LOG(LOG_INFO, args)
#define DBG(args...)  DO_LOG(LOG_DEBUG, args)
#define ERR(args...)  DO_LOG(LOG_ERR, args)

#endif /* __MSG_H__ */
