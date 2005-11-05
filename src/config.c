/* config.c - Interfaces with libconf and handles all program setup.
 *
 * Copyright (C) 2003, 2004 Joachim Nilsson <joachim!nilsson()member!fsf!org>
 *
 * GNU Alive is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * GNU Alive is distributed in the hope that it will be useful, but
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

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

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
  {{"SERVER_PORT", "PORT", NULL},                      NULL, PORT},

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

/* Fallback PID files, in order after PID_FILE
 * Check in reverse order.
 */
char *fallback_pid_files[] = {"/tmp/alive.pid",
                              "~/alive.pid",
                              "/tmp/qadsl.pid",
                              "~/qadsl.pid",
                              NULL};

/* What conf file to use:
 *   1. Command line option
 *   2. ~/.aliverc
 *   3. ~/.qadslrc
 *   4. GLOBAL_CONF, i.e., default value set at configure time.
 *   5. /etc/alive.conf
 *   6. /etc/qadsl.conf
 */
char *possible_conf_files[] =
  {
    "~/.aliverc",
    "~/.qadslrc",
    GLOBAL_CONF,
    "/etc/alive.conf", /* XXX - Obsolete, GLOBAL_CONF is always correct? */
    "/etc/qadsl.conf",
    NULL
  };

/**
 * is_file_available - Answers the question if the file exists.
 * @file: File to look for.
 *
 * Returns: Numerical one (1) if @file exists, otherwise zero (0).
 */

static int is_file_available (char *file)
{
  int result = 0;
#if 1
  struct stat f;

  if (file)
    {
      result = stat (file, &f);
      if (result)
        {
          result = 0;
        }
      else
        {
          /* If @file is a regular file with either owner, group or others
             readability */
          if (S_ISREG(f.st_mode)
              &&
              ((f.st_uid == getuid() && (f.st_mode & S_IRUSR))
               ||
               (f.st_gid == getgid() && (f.st_mode & S_IRGRP))
               ||
               (f.st_mode & S_IROTH)))
            {
              result = 1;
            }
        }
    }
#else  /* This code segfaults at fclose() on Debian unstable, glibc-2.3.5-7 */
  FILE *fp;
  fp = fopen (file, "r");
  if (fp)
    {
      result = 1;
      fclose (fp);
    }
#endif
  return result;
}


/**
 * tilde_expand - Expands ~/ to /home/$LOGNAME/.
 * @file: File path to expand.
 *
 * This method simply replaces the ~/ in a @file path,
 * but only if the first char is ~.  If not the method
 * returns a strdup()'ed @file string.
 *
 * Returns: NULL only when memory is exhausted, otherwise it
 *          returns a malloc'ed tilde expanded file name.
 */

static char *tilde_expand (char *file)
{
  size_t len;
  char  *new_file;
  char  *user_home = getenv("HOME");

  assert (file);
  assert (user_home);

  DBG("file=%s\n", file);
  if ('~' != file[0])
    {
      /* Not a tilde directory... pretend it was. */
      return strdup (file);
    }

  len = strlen (user_home) + strlen (file);
  new_file = (char *)malloc (len);
  if (!new_file)
    {
      ERR("Failed allocating space for file name");
      return NULL;
    }

  strncpy (new_file, user_home, strlen (user_home) + 1);
  strncat (new_file, &file[1], strlen (file));

  return new_file;
}

/**
 * config_locate - Returns name of conf file to use.
 * @file: Command line specified conf file, or NULL if none.
 *
 * This method returns the name of the conf file to use.  It achieves
 * this by first inspecting the input file, and if that is invalid
 * moves on to try and find the highest prioritized file.
 *
 * Returns: The name of the config file to use.
 */

static char *config_locate (char *file)
{
  if (file)
    {
      if (is_file_available(file))
        {
          __config_area.conf_file = strdup (file);
        }
      else
        {
          ERR("Requested CONF file: \"%s\" does not exist.", file);
          file = NULL;
        }
    }

  /* If no file was specified or @file didn't exist... */
  if (!file)
    {
      int i;

      for (i = 0; possible_conf_files [i]; i++)
        {
          DBG("%d=%s\n", i, possible_conf_files[i]);
          file = tilde_expand (possible_conf_files[i]);
          if (!file)
            {
              ERR("Failed expanding conf file %d\n", i);
            }
          else
            {
              DBG("Looking for CONF file: %s", file);
              if (is_file_available (file))
                {
                  __config_area.conf_file = file;
                  break;
                }
              free (file);
            }
        }
    }

  if (!__config_area.conf_file)
    {
      return NULL;
    }

  /* Old config file? */
  if (strstr (file, "qadsl"))
    {
      char *pos, *temp = strdup (file);
      pos = strstr (temp, "qadsl");
      memcpy (pos, "alive", 5);

      ERR("Old conf file: %s, rename it to %s.", file, temp);
      free (temp);
    }
  else
    {
      DBG("Using %s for configuration data.", file);
    }

  return __config_area.conf_file;
}

void print_parm (param_t *p)
{
  if (!p) return;

  DBG("%s = %s", p->names[0], p->value);
}

config_data_t * config_load (char *file)
{
  int result;
  char *temp;

  /* Setup default configuration */
  file = config_locate (file);
  if (!file)
    {
      ERR("Cannot find any config file, please create one.\n");
      return NULL;
    }

  /* Fill in the blanks by reading the conf file */
  result = conf_read_file (parms, file);
  if (result < 0)
    {
      ERR(_("Cannot find configuration file %s: %s\n"),
             file, strerror (errno));
      return NULL;
    }

  if (IS_DBG())
  {
    int i;

    DBG(_("Read configuration:"));
    for (i = 0; i < (sizeof (parms) / sizeof (parms[0])); i++)
      {
        print_parm (&parms[i]);
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
      __config_area.daemon_type  = 1; /* Always default to daemon mode. */
    }
  else
    {
      __config_area.daemon_type  = (strncasecmp (temp, "login", 5) == 0) ? 1 : 0;
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
          ERR(_("Failed to read username from configuration file %s.\n"), file);
        }
      else
        {
          ERR(_("Failed to read password from configuration file %s.\n"), file);
        }

      ERR(_("You must supply at least a username and password.\n"));
      return NULL;
    }

  return &__config_area;
}

/* Local Variables:
 * mode: C;
 * c-file-style: gnu;
 * indent-tabs-mode: nil;
 * End:
 */
