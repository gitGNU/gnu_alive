/* config.h - Handles all program setup */
#ifndef __CONFIG_H__
#define __CONFIG_H__

/* Include configure settings */
#include "settings.h"


struct config_data
{
  /* The config file, usually /etc/qadsl.conf */
  char *conf_file;

  /* The PID file, usually /var/run/qadsl.pid */
  char *pid_file;

  /* Socket we're connected to the login_server with. */
  int  sockfd;
  char *login_server;
  int   server_port;

  char *init_page;
  char *login_page;
  char *logout_page;

  int  logged_in;
  char send_msg[MAXDATASIZE], get_msg[MAXDATASIZE];

  char *username;
  char *password;

  int daemon_start;
  int daemon_type;
  int daemon_delay;
};

typedef struct config_data config_data_t;

config_data_t *config_load (char *file);

#endif	/* __CONFIG_H__ */
