/* conf.c -  
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <stdio.h>
#include <stdio.h>
#include <fcntl.h>		/* old method of redirecting stdin */
#include <sys/stat.h>		/* old method of redirecting stdin */
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "conf.h"

#if 0  /* XXX - not yet reentrant */
extern int yyparse (void *arg);
#else
extern int yyparse (void);
#endif

param_t *__current_list;

/**
 * conf_read_file - Reads a .conf file and tries to set parameters in the given list. 
 * @parameter_list:
 * @file:
 *
 * Reads a .conf file and tries to set parameters in the given list. 
 *
 * Returns:
 */

int
conf_read_file (param_t *parameter_list, char *file)
{
  int    fd, stdin_fd; 
  FILE   *fp; 			/* Might be used later when NetBSD works... */
  param_t *p;
  int result;

  if (!parameter_list)
    {
      errno = EINVAL;		/* Bogus parameter */
      return -1;
    }

   stdin_fd = fileno (stdin);   /* save descriptor for 'stdin' */ 
   fd       = dup (stdin_fd); 
 
   if (fd == -1) 
      return -1;               /* failed to duplicate input descriptor */ 
 
#if 0 /* Does not work on NetBSD 1.6 */
   /* use the duplicated descriptor to redirect input... */ 
   fp = fdopen (fd, "r"); 
 
   if (!fp) 
      return -2;               /* failed to open duplicated descriptor */ 
 
   stdin = freopen (file, "r", fp);

   if (!stdin) 
      return -3;               /* failed to redirect stream input */ 
#else /* Revert to older method */
   {
     int temp_fd;

     temp_fd = open (file, O_RDONLY, S_IREAD);
     if (-1 == temp_fd)
       return -2;

     dup2 (temp_fd, stdin_fd);

     close (temp_fd);
   }
#endif

#if 0 /* Does not work with yacc, revert while we wait
       * for libconf to become reentrant. */ 
   result = yyparse ((void *)parameter_list);
#else
   __current_list = parameter_list;
   result = yyparse ();
#endif

   /* UNDO: now undo the effects of redirecting input... */ 
#if 0 /* Does not work on NetBSD 1.6 */
   fclose(stdin); 
 
   stdin = fdopen (stdin_fd, "r"); 
#else /* Revert to older method */
   dup2 (fd, stdin_fd);
   close (fd);
#endif
 
   if (!stdin) 
      return -4;               /* failed to reestablish 'stdin' */ 
 
  /* Setup defaults */
  p = parameter_list;
  while (p->names[0])
    {
      if (!p->value)
	{
	  p->value = p->default_value;
	}
      p++;
    }

  return 0;
}


/**
 * conf_find_key - Finds a matching key in the given parameter list.
 * @parameter_list: List of parameters.
 * @key:            The key to be located
 *
 * Tries to find a matching @key in the given @paramter_list. If
 * unsuccessful it returns NULL.
 *
 * Returns: Pointer to the located key, or NULL on failure.
 */

static param_t *
conf_find_key (param_t *parameter_list, char *key)
{
  param_t *parm = parameter_list;
  char **keys;

  if (!parm)
    {
      fprintf (stderr, "conf_read_file() did not get a correct parameter list.\n");
      return NULL;
    }

  /* A list is either NULL terminated or the names list is empty. */
  while (parm && parm->names && parm->names[0])
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


/* Our very own strndup(), this should go into a compatibility lib. */
static char *
strlndup (char *src, size_t len)
{
  char *dst;

  if (!src)			/* Cannot copy non-existing string. */
    return NULL;

  dst = malloc (len + 1);
  if (!dst)
    return NULL;

  strncpy (dst, src, len);

  dst[len] = '\0';

  return dst;
}


/**
 * strlncpy - Rip-off from OpenBSD's strlcpy()
 * @dst: Destination string.
 * @src: Source string.
 * @len: maximum number of char's to copy.
 *
 * XXX - Relies on dst being valid 
 * Makes sure to NUL terminate dst when done.
 */

static size_t
strlncpy (char *dst, char *src, size_t len)
{
  size_t src_len = strlen (src);

  if (src_len > len)
    src_len = len;

  strncpy (dst, src, src_len);

  dst[src_len] = '\0';

  return src_len;
}


/* TODO: Implement strlncat() or setup proper aliases for 
 * OpenBSD's strlcpy() strlcat() if configure does not find
 * support for them or libsafe <--- Lucent contribution.
 */


/* XXX - what if the key doesn't exist?
 * XXX - signal for bogus config file?
 */
int
conf_set_value (param_t *parameter_list, char *key, char *value)
{
  int result;
  param_t *parm;
  char    *orig_value = value;

  /* Check if Flex failed to allocate a new string. */
  if (!key || !value)
    {
      fprintf (stderr,
	       "Failed to allocate memory for configuration values: %s\n",
	       strerror (errno));
      return -1;
    }

  /* XXX - temporary fix to allow use of yacc instead of bison */
  if (!parameter_list)
    parameter_list = __current_list;

  parm = conf_find_key (parameter_list, key);
  if (parm)
    {
      int len = strlen (value);
      char *str;

      /* Cleanup of argument. It can be any of the following:
       * value     len=6
       * "value"   len=8
       * value;"   len=7
       * "value";  len=9
       */
      if (len > 0 && value [len - 1] == ';')
	len --;

      if (len > 1 && value [0] == '"' && value [len - 1] == '"')
	{
	  len -= 1;
	  value++;
	}

      str = strlndup (value, len);

      /* Free the string strdup():ed by flex, not the one
       * we might have been messing around with, pointer-wise.
       * XXX - Interface-wise this is not very good, flex should do it.
       */
      free (orig_value);

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
conf_get_value (param_t *parameter_list, char *key)
{
  param_t *parm;
  char *result = NULL;

  parm = conf_find_key (parameter_list, key);
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
conf_get_bool (param_t *parameter_list, char *key)
{
  bool result = false;
  char *value = NULL;

  value = conf_get_value (parameter_list, key);
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
