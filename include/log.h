#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>


void write_log (char *fmt, ...);

#define LOG(args...)				\
  if (verbose == 2)				\
    printf (args);				\
  else if (verbose == 1)			\
    write_log (args);				\
  else						\
    {}

#define ERROR(args...)				\
  if (verbose == 2)				\
    fprintf (stderr, args);			\
  else if (verbose == 1)			\
    write_log (args);				\
  else						\
    {}

#endif /* __LOG_H__ */
