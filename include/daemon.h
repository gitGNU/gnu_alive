#ifndef __DAEMON_H__
#define __DAEMON_H__

#include <sys/types.h>


int  daemonize (void);
void daemon_thread (config_data_t *config);
int  daemon_kill (pid_t pid);


#endif  /* __DAEMON_H__ */
