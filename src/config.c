/* config.c - Interfaces with libconf and handles all program setup.
 *
 * Copyright (C) 2003, 2004 Joachim Nilsson <joachim!nilsson()member!fsf!org>
 *
 * qADSL is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * qADSL is distributed in the hope that it will be useful, but
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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "conf.h"
#include "log.h"


/* Default values are for a standard Orbyte Wireless server.
 * Basically you only need to supply your username and a password
 * in the qadsl.conf file.
 */
param_t parms [] = {
  {{"USER", "USERNAME", NULL},                         NULL, NULL},
  {{"PASS", "PASSWORD", NULL},                         NULL, NULL},
  {{"SERV", "SERVER", "LOGIN_SERVER", NULL},           NULL, "10.0.0.6"},
  /* Default HTTP connection port, usually port 80 */
  {{"SERVER_PORT", NULL},                              NULL, PORT},

  /* This builds the internet_login() login_string. */
  {{"LOGIN_STRING_HEADER", "LOGIN_DATA_HEADER", NULL}, NULL, NULL},
  {{"USERNAME_KEY", NULL},                             NULL, "username"},
  {{"PASSWORD_KEY", NULL},                             NULL, "password"},
  {{"LOGIN_STRING_FOOTER", "LOGIN_DATA_FOOTER", NULL}, NULL, "submitForm=Login"},

  /* Subtext to look for as result of a successful login */
  {{"LOGGED_IN_STRING", NULL},                         NULL, "/sd/create_session_window"}, /* "newPane()" */
  /* Subtext to look for as result of a successful logout */
  {{"LOGGED_OUT_STRING", NULL},                        NULL, "Login again"}, /* "avslutat" */

  /* Pre login page */
  {{"INIT_PAGE", "INIT", NULL},                        NULL, "/sd/init"},
  /* Login form */
  {{"LOGIN_PAGE", "LOGIN", NULL},                      NULL, "/sd/login"},
  /* Logout page */
  {{"LOGOUT_PAGE", "LOGOUT", NULL},                    NULL, "/sd/logout"},

  /* Lock file */
  {{"PID_FILE", NULL},                                 NULL, PID_FILE},

  /* Default: start the keep-alive daemon */
  {{"DEAMON_S", "DEAMON_START", "DAEMON_START", "START_DEAMON", "START_DAEMON", NULL}, NULL, "true"},
  /* Default: re-login every DAEMON_DELAY minutes. */
  {{"DEAMON_T", "DEAMON_TYPE", "DAEMON_TYPE", NULL},  NULL, "login"},
  /* Default: relogin every 5 minutes to keep the connection alive. */
  {{"DEAMON_D", "DEAMON_DELAY", "DAEMON_DELAY", "INTERVAL", NULL}, NULL, "5"},

  /* NULL terminate for libconf. */
  {{NULL}, NULL, NULL}
};

/* This is for global data. */
static config_data_t __config_area;

/* Fallback PID files, in order after PID_FILE */
char *fallback_pid_files[] = {"/tmp/qadsl.pid", "~/qadsl.pid", NULL};


static char *
config_locate (char *file)
{
  char *user_home;

  /* What conf file to use:
   *   ~/.qadslrc,
   *   /etc/qadsl.conf or something
   *   from the command line?
   */
  if (!file)
    {
      __config_area.conf_file = strdup (GLOBAL_CONF);

      if (strncmp ("/", USER_CONF, 1) != 0)
        {
          user_home = getenv ("HOME");
          if (user_home)
            {
              FILE   *file;
              size_t  len;
              char   *filename, *user_conf;

              user_conf = USER_CONF;
              len       = strlen (user_home) + strlen (user_conf) + 2;
              filename  = (char *)malloc (len);
              if (filename)
                {
                  strcpy (filename, user_home);

                  if (strncmp ("~/", user_conf, 2) == 0)
                    {
                      user_conf++;
                    }

                  strcat (filename, user_conf);

                  /* If ~/.qadslrc exists replace __config_area.conf_file */
                  if (NULL != (file = fopen (filename, "")))
                    {
                      fclose (file);
                      free (__config_area.conf_file);
                      __config_area.conf_file = filename;
                    }
                }
            }
        }
    }
  else
    {
      __config_area.conf_file = strdup (file);
    }

  if (!__config_area.conf_file)
    {
      return NULL;
    }

  return __config_area.conf_file;
}

void print_parm (param_t *p, int verbose)
{
  if (!p)
    return;

  DEBUG ("%s = %s", p->names[0], p->value);
}

config_data_t *
config_load (char *file, int verbose)
{
  int result;
  char *temp;

  /* Setup default configuration */
  file = config_locate (file);

  /* Fill in the blanks by reading the conf file */
  result = conf_read_file (parms, file);
  if (result < 0)
    {
      ERROR ("Cannot find configuration file %s: %s\n",
             file, strerror (errno));
      return NULL;
    }

  if (IS_DEBUG(verbose))
  {
    int i;

    DEBUG ("Read configuration:");
    for (i = 0; i < (sizeof (parms) / sizeof (parms[0])); i++)
      {
        print_parm (&parms[i], verbose);
      }
  }

  /* Setup the rest of the default settings */
  __config_area.login_server = conf_get_value (parms, "SERV");
  __config_area.init_page    = conf_get_value (parms, "INIT_PAGE");
  __config_area.login_page   = conf_get_value (parms, "LOGIN_PAGE");
  __config_area.logout_page  = conf_get_value (parms, "LOGOUT_PAGE");
  __config_area.pid_file     = conf_get_value (parms, "PID_FILE");

  __config_area.login_string_header = conf_get_value (parms, "LOGIN_STRING_HEADER");
  __config_area.username_key        = conf_get_value (parms, "USERNAME_KEY");
  __config_area.password_key        = conf_get_value (parms, "PASSWORD_KEY");
  __config_area.login_string_footer = conf_get_value (parms, "LOGIN_STRING_FOOTER");

  __config_area.logged_in_string  = conf_get_value (parms, "LOGGED_IN_STRING");
  __config_area.logged_out_string = conf_get_value (parms, "LOGGED_OUT_STRING");

  temp = conf_get_value (parms, "SERVER_PORT");
  if (temp)
    {
      __config_area.server_port = atoi (temp);
    }
  else
    {
      __config_area.server_port = atoi (PORT);
    }

  __config_area.daemon_start = conf_get_bool (parms, "DEAMON_S");

  temp = conf_get_value (parms, "DEAMON_T");
  if (!temp)
    {
      __config_area.daemon_type  = 1;
    }
  else
    {
      __config_area.daemon_type  = strcasecmp (temp, "login") ? 1 : 0;
    }

  temp = conf_get_value (parms, "DEAMON_D");
  if (!temp)
    {
      /* XXX - hardcoded to fix buggy conf_get_value() that returns NULL when no conf file is found. */
      __config_area.daemon_delay = 20;
    }
  else
    {
      __config_area.daemon_delay = atoi (temp);
    }

  __config_area.username     = conf_get_value (parms, "USER");
  __config_area.password     = conf_get_value (parms, "PASS");

  if (!__config_area.username || !__config_area.password)
    {
      if (!__config_area.username)
        {
          ERROR ("Failed to read username from configuration file %s.\n", file);
        }
      else
        {
          ERROR ("Failed to read password from configuration file %s.\n", file);
        }

      ERROR ("You must supply at least a username and password.\n");
      return NULL;
    }

  return &__config_area;
}
