/* config.h - Handles all program setup */
#ifndef __CONFIG_H__
#define __CONFIG_H__

/* Include configure settings */
#include "settings.h"

#ifdef HAVE_FEATURES_H
#include <features.h>
#endif

/* Taken in part from zsh.h */
#ifdef __hpux
// pull in off_t
#define _INCLUDE_POSIX_SOURCE
#define _INCLUDE_XOPEN_SOURCE
#define _INCLUDE_HPUX_SOURCE
#include <sys/types.h>
#endif
                                                                                               
struct config_data
{
  char *username;
  char *password;
  char *login_server;
  int   server_port;

  /* These four build the login_string in http.c:internet_login() */
  char *login_string_header;
  char *username_key;
  char *password_key;
  char *login_string_footer;

  /* Search reply for successful login/logout */
  char *logged_in_string;
  char *logged_out_string;

  char *init_page;
  char *login_page;
  char *logout_page;

  /* Are we logged in? */
  int  logged_in;

  /* Disgusting static communication buffers */
  char send_msg[MAXDATASIZE], get_msg[MAXDATASIZE];

  /* Start the keep-alive daemon? */
  int daemon_start;
  /* "Ping" or login every @daemon_delay minutes? */
  int daemon_type;
  /* Sleep for X minutes before pinging or logging in again. */
  int daemon_delay;

  /* The config file, usually /etc/qadsl.conf */
  char *conf_file;

  /* The PID file, usually /var/run/qadsl.pid */
  char *pid_file;

  /* Socket we're connected to the login_server with. */
  int  sockfd;
};

typedef struct config_data config_data_t;

config_data_t *config_load (char *file);

#endif	/* __CONFIG_H__ */
