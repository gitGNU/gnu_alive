#ifndef __LOCK_H__
#define __LOCK_H__

#include <sys/types.h>


int   lock_create (char **file, pid_t pid);
pid_t lock_read   (char **file);
int   lock_remove (char *file);


#endif /* __LOCK_H__ */
