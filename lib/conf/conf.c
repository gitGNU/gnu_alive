#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "conf.h"

extern int yyparse (void);

static param_t *parms;


int
conf_read_file (char *file, param_t *parameters)
{
  param_t *p;
  int result;
  extern FILE *yyin;

  /* Setup the table of valid keys */
  parms = parameters;
  if (!parms)
    {
      errno = EINVAL;		/* Bogus parameter */
      return -1;
    }

  yyin = fopen (file, "r");
  if (!yyin)
    {
      return -1;
    }

  result = yyparse ();

  /* Setup defaults */
  p = parms;
  while (p->names[0])
    {
      if (!p->value)
	{
	  p->value = p->default_value;
	}
      p++;
    }
}


static param_t *
conf_find_key (char *key)
{
  int i;
  param_t *parm = parms;
  char **keys;

  if (!parm)
    {
      fprintf (stderr, "conf_read_file() did not get a correct parameter list.\n");
      return NULL;
    }

  while (parm)
    {
      keys = parm->names;
      while (*keys)
	{
	  if (strcasecmp (key, *keys) == 0)
	    return parm;

	  keys ++;
	}

      parm ++;
    }

  return NULL;
}


/* XXX - what if the key doesn't exist?
 * XXX - signal for bogus config file?
 */
int
conf_set_value (char *key, char *value)
{
  int result;
  param_t *parm;

  /* Check if Flex failed to allocate a new string. */
  if (!key || !value)
    {
      fprintf (stderr,
	       "Failed to allocate memory for configuration values: %s\n",
	       strerror (errno));
      return -1;
    }
  
  parm = conf_find_key (key);
  if (parm)
    {
      int len = strlen (value);
      char *str;

      /* Cleanup of argument. It can be any of the following:
       * value
       * "value"
       * value;
       * "value";
       */
      if (value [len - 1] == ';')
	len --;

      if (value [0] == '"' && value [len - 1] == '"')
	{
	  len -= 2;
	  str = strndup (++value, len);
	}
      else
	{
	  str = strndup (value, len);
	}

      free (value);

      parm->value = str;
      result  = 0;
    }
  else
    {
      result  = -1;
    }

  return result;
}


/**
 * conf_get_value - Get value of config parameter
 * @key: Search key for config parameter.
 *
 * This function searches the database of configuration parameters
 * for the given parameter, @key and returns its string value.
 *
 * Returns: The string value of the parameter @key or
 *          NULL if the @key does not exist.
 */

char *
conf_get_value (char *key)
{
  param_t *parm;
  char *result = NULL;

  parm = conf_find_key (key);
  if (parm)
    {
      result = parm->value;
    }

  return result;
}


/**
 * conf_get_bool - Get boolean value of config parameter.
 * @key: Search key for config parameter.
 *
 * Similar in function to conf_get_value() but also evaluates
 * the parameter value to determine the boolean value.
 *
 * Returns: TRUE if key exists and is set to true, yes or on.
 *          FALSE if the key does not exist or is false, no or off.
 */

bool
conf_get_bool (char *key)
{
  bool result = false;
  char *value = NULL;

  value = conf_get_value (key);
  if (!value)
    {
      result = FALSE;
    }
  else
    {
      if (strcasecmp (value, "TRUE") == 0)
	result = true;
      else if (strcasecmp (value, "YES") == 0)
	result = true;
      else if (strcasecmp (value, "ON") == 0)
	result = true;
      else
	result = false;
    }

  return result;
}


/* The new config file parser should easily parse even the old 
 * format, but if that does not work, for some reason, enabling
 * this would fix the problem.
 *
 * --enable-old-config-parser
 */

#ifdef OLD_CONFIG_PARSER
void
conf_read (conf_data_t *config)
{
  FILE *fd;
  char input[MAXLEN], tmp[MAXLEN], *p;
  int user_sw = 0, host_sw = 0, pass_sw = 0, dea_s_sw = 0, dea_t_sw =
    0, dea_d_sw = 0;

  fd = fopen (conf, "r");
  if (fd == NULL)
    {
      perror ("Could not open configuration file\n");
      exit (1);
    }

  while ((fgets (input, sizeof (input), fd)) != NULL)
    {
      if ((p = strchr (input, '\n')))
	*p = '\0';

      if ((strncmp ("SERV=", input, 5)) == 0)
	{
	  strcpy (serv, input + 5);
	  host_sw = 1;
	}

      if ((strncmp ("USER=", input, 5)) == 0)
	{
	  strcpy (tmp, input + 5);
	  url_encode (user, tmp, strlen (tmp));
	  user_sw = 1;
	}

      if ((strncmp ("PASS=", input, 5)) == 0)
	{
	  strcpy (tmp, input + 5);
	  url_encode (pass, tmp, strlen (tmp));
	  pass_sw = 1;
	}

      if ((strncmp ("DEAMON_S=", input, 9)) == 0)
	{
	  strcpy (deamon_s, input + 9);
	  dea_s_sw = 1;
	}

      if ((strncmp ("DEAMON_T=", input, 9)) == 0)
	{
	  strcpy (deamon_t, input + 9);
	  dea_t_sw = 1;
	}

      if ((strncmp ("DEAMON_D=", input, 9)) == 0)
	{
	  strcpy (deamon_d, input + 9);
	  dea_d_sw = 1;
	}

    }

  if ((!user_sw || !pass_sw || !host_sw || !dea_s_sw || !dea_t_sw
       || !dea_d_sw))
    {
      printf ("Error found in: %s\n", conf);

      if (!user_sw)
	{
	  fprintf (stderr, "\tCould not find username!\n");
	  exit (1);
	}
      if (!pass_sw)
	{
	  fprintf (stderr, "\tCould not find password!\n");
	  exit (1);
	}
      if (!host_sw)
	{
	  fprintf (stderr, "\tCould not find login server!\n");
	  exit (1);
	}
      if (!dea_s_sw)
	{
	  fprintf (stderr, "\tCould not find deamon startup choice!\n");
	  exit (1);
	}
      if (!dea_t_sw)
	{
	  fprintf (stderr, "\tCould not find deamon type (LOGIN/PING)!\n");
	  exit (1);
	}
      if (!dea_d_sw)
	{
	  fprintf (stderr, "\tCould not find deamon delay!\n");
	  exit (1);
	}

    }
}
#endif 
