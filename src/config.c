/* config.c - Interfaces with libconf and handles all program setup. */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "conf.h"
#include "config.h"


/* Default values are for a standard Orbyte Wireless server.
 * Basically you only need to supply your username and a password 
 * in the qadsl.conf file.
 */
param_t parms [] = {
  {{"USER", "USERNAME", NULL},                         NULL, NULL},
  {{"PASS", "PASSWORD", NULL},                         NULL, NULL},
  {{"SERV", "SERVER", "LOGIN_SERVER", NULL},           NULL, "10.0.0.6"},
  {{"SERVER_PORT", NULL},                              NULL, PORT},
  {{"PRE_LOGIN_DATA", NULL},                           NULL, ""},
  {{"USERNAME_KEY", NULL},                             NULL, "username"},
  {{"PASSWORD_KEY", NULL},                             NULL, "password"},
  {{"POST_LOGIN_DATA", NULL},                          NULL, "submitForm=Logga in"},
  {{"LOGOUT_DATA", NULL},                              NULL, "avslutat"},
  {{"INIT_PAGE", NULL},                                NULL, "/sd/init"},
  {{"LOGIN_PAGE", NULL},                               NULL, "/sd/login"},
  {{"LOGOUT_PAGE", NULL},                              NULL, "/sd/logout"},
  {{"PID_FILE", NULL},                                 NULL, PID_FILE},
  {{"DEAMON_S", "DAEMON_START", "START_DAEMON", NULL}, NULL, "true"},
  {{"DEAMON_T", "DAEMON_TYPE", NULL},                  NULL, "login"},
  {{"DEAMON_D", "DAEMON_DELAY", "INTERVAL", NULL},     NULL, "20"},
  NULL
};

/* This is for global data. */
static config_data_t __config_area;


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

void print_parm (param_t *p)
{
  if (!p)
    return;

  printf ("%s = %s\n", p->names[0], p->value);
}

config_data_t *
config_load (char *file)
{
  int i;
  char *temp;

  /* Setup default configuration */
  file = config_locate (file);

  /* Fill in the blanks by reading the conf file */
  if (-1 == conf_read_file (file, parms))
    {
      fprintf (stderr, "Cannot find configuration file %s: %s\n", 
               file, strerror (errno));
      return NULL;
    }

#ifdef DEBUG
  printf ("Read configuration:\n");  
  for (i = 0; i < (sizeof (parms) / sizeof (parms[0])); i++)
    {
      print_parm (&parms[i]);
    }
#endif	/* DEBUG */

  /* Setup the rest of the default settings */
  __config_area.login_server = conf_get_value ("SERV");
  __config_area.init_page    = conf_get_value ("INIT_PAGE");
  __config_area.login_page   = conf_get_value ("LOGIN_PAGE");
  __config_area.logout_page  = conf_get_value ("LOGOUT_PAGE");
  __config_area.pid_file     = conf_get_value ("PID_FILE");
  
  temp = conf_get_value ("SERVER_PORT");
  if (temp)
    {
      __config_area.server_port = atoi (temp);
    }
  else
    {
      __config_area.server_port = atoi (PORT);
    }

  __config_area.daemon_start = conf_get_bool ("DEAMON_S");
  __config_area.daemon_type  = strcasecmp (conf_get_value ("DEAMON_S"), "login") ? 1 : 0;
  __config_area.daemon_delay = atoi (conf_get_value ("DEAMON_D"));

  __config_area.username     = conf_get_value ("USER");
  __config_area.password     = conf_get_value ("PASS");

  if (!__config_area.username || !__config_area.password)
    {
      if (!__config_area.username)
	fprintf (stderr,
		 "Failed to read your username from qADSL configuration file %s.\n",
		 file);
      else
	fprintf (stderr,
		 "Failed to read your password from qADSL configuration file %s.\n",
		 file);
	
      fprintf (stderr, "You must supply at least a username and password.\n");
      return NULL;
    }

  return &__config_area;
}
