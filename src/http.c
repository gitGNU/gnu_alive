/* http.c -  */
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>

#include "config.h"
#include "http.h"
#include "log.h"


/**
 * url_encode - Hexifies a character string for URL use.
 * @dest: A destination buffer.
 * @src:  The string to be encoded.
 * @len:  Length of @src.
 *
 * This function scans @src for unsafe characters and converts
 * them into %XX entries. This means that @dest must be large
 * enough to hold this expanded string from @src.
 *
 * Returns: Number of bytes written to @dest, should be >= @len
 *          any number less than or zero indicates an error.
 */

static unsigned int
url_encode (char *dest, const char *src, unsigned int len) 
{
  const char unsafe[]=" %<>\"#{}|\\^~[]`;/?:@=&";
  register const unsigned char* s = src;
  unsigned long written=0,i;

  inline tohex (const unsigned char c)
    {
      return c > 9 ? c - 10 + 'A' : c + '0';
    }

  if (!dest)
    {
      /* Eh, no destination? */
      return 0;
    }

  for (i = 0; i < len; i++) 
    {
      /* Upper half of ASCII table or one of the unsafe chars? */
      //if (s[i] & 0x7F || strchr (unsafe, s[i]))
      /* Simple, just trap unsafe characters */
      if (strchr (unsafe, s[i]))
        {
          if (' ' == s[i])
            {
              /* Spaces to plus */
              dest[written] = '+';
            }
          else
            {
              dest[written]   = '%';
              dest[written+1] = tohex (s [i] >> 4);
              dest[written+2] = tohex (s [i] & 15);
              written += 3;
            }
        } 
      else 
        {
          dest [written] = s [i];
          written ++;
        }
    }

  return written;
}


/**
 * open_server - 
 * @name: 
 * @port:
 * @verbose: 
 *
 * Returns: -1 and errno on error, otherwise
 *          socket descriptior.
 *
 */

int 
open_server (char *name, short port, int verbose)
{
  int result, sockfd;
  struct hostent *he;
  struct sockaddr_in address;
  

  /* Try to find the login host */
  LOG("Looking up %s ... ", name);

  he = gethostbyname (name);
  if (he == NULL)
    {
      ERROR("Cannot find login host: %s\n", name);
      result = -1;
    }
  else
    {
      /* Setup a socket */
      LOG("0x%X\n", he->h_addr);

      sockfd = socket (PF_INET, SOCK_STREAM, 0);
      if (-1 == sockfd)
        {
          ERROR("Failed to create a socket: %s\n", strerror (errno));
          result = -1;
        }
      else
        {
          /* Setup address to connecto to ... */
          address.sin_family = PF_INET;
          address.sin_port   = htons (port);
          address.sin_addr   = *((struct in_addr *) he->h_addr);
          memset (&(address.sin_zero), 0, sizeof (address.sin_zero));
	  
          /* Connect to login server */
          if (verbose)
            {
              LOG("Connecting to %s(0x%X)\n", name, he->h_addr);
            }
          result = connect (sockfd, (struct sockaddr *) &address, sizeof (address));
          if (-1 == result && verbose)
            {
              ERROR("Failed to connect to login host: %s\n\t==>%s",
                    name, strerror (errno));
            }
          else
            {
              result = sockfd;
            }
        }
    }

  return result;
}


/**
 * pre_login - Bring up Orbyte login screen
 * @config: Pointer to config data area.
 * @sockfd: Socket we are connected to.
 *
 * This function serves as a sort of "ping" to ensure that
 * the login server is alive and accepting connections.
 *
 * In reality the %PRELOGIN_MSG must be sent to initiate the
 * actual login page at the server.
 *
 * Returns: 0 if login server alive, otherwise a negative value.
 */

int
pre_login (config_data_t *config, int verbose)
{
  int result, tries;

  /* Open a connection to the login server */
  config->sockfd = open_server (config->login_server, config->server_port, verbose);
  if (config->sockfd < 0)
    {
      ERROR ("Failed to connect to login server: %s\n",
             strerror (errno));
      return -1;
    }


  /* Register and get login page */
  sprintf (config->send_msg, PRELOGIN_MSG,
           config->init_page,
           config->login_server,
           config->login_server);

#ifdef DEBUG
  if (verbose)
    {
      LOG ("Sent:\n%s\n", config->send_msg);
    }
#endif

  result = send (config->sockfd,
                 config->send_msg,
                 strlen (config->send_msg), 0);


  /* Read reply */
  /* In a not so distant future this function will be extended to
   * include a very small HTML parser. It will wait for completion
   * of the send() command and then parse the file at sockfd to
   * find the username, password and any extra data to the login
   * string used later by internet_login()
   */

  tries = 0;
  do 
    {
      sleep (2 * tries);
      result = read (config->sockfd, config->get_msg, sizeof (config->get_msg));
    }
  while (-1 == result && tries++ < MAX_RETRIES);

  if (-1 == result)
    {
      ERROR("Read reply: %s\n", strerror (errno));
    }
  else
    {
#ifdef DEBUG
      if (verbose)
        {
          LOG ("Received:\n%s", config->get_msg);
        }
#endif
      result = 0;
    }

  close (config->sockfd);

  return result;
}


/**
 * internet_login - Handles the login phase.
 * @config:  Pointer to config data area.
 * @sockfd:  Socket we are connected to.
 * @verbose: Log as much as possible.
 * 
 * This is the function that handles the logging in to the
 * Internet gateway of the ISP.
 *
 * Returns: 0 if login was successful, otherwise -1.
 */

int
internet_login (config_data_t *config, int verbose)
{

  int result, tries, length;
  char *login_string, *temp;

  /* Open a connection to the login server */
  config->sockfd = open_server (config->login_server, config->server_port, verbose);
  if (config->sockfd < 0)
    {
      ERROR("Failed to connect to login server: %s\n",
            strerror (errno));
      return -1;
    }
#if 0
  result = sprintf (login_string, LOGIN_STRING,
                    config->username, config->password);
#endif
  length = (config->login_string_header ? strlen (config->login_string_header) : 0)
    + strlen (config->username_key) + strlen (config->username)
    + strlen (config->password_key) + strlen (config->password) 
    + strlen (config->login_string_footer) + strlen ("Plus approximately 10%");

  temp = (char *) malloc (length);
  if (!temp)
    {
      ERROR ("Failed to allocate memory: %s\n",
             strerror (errno));
      return -1;
    }

  result = 0;
  if (config->login_string_header)
    result = sprintf (temp, "%s&", config->login_string_header);
  
  result += sprintf (&temp[result], "%s=%s&%s=%s",
                     config->username_key,
                     config->username,
                     config->password_key,
                     config->password);

  if (config->login_string_footer)
    result += sprintf (&temp[result], "&%s", config->login_string_footer);
  
#ifdef DEBUG
  if (verbose)
    {
      LOG("Non-url-encoded login string:\n%s\n", temp);
    }
#endif

  login_string = (char *) malloc (length);
  if (!login_string)
    {
      free (temp);
      ERROR ("Failed to allocate memory: %s\n",
             strerror (errno));
      return -1;
    }
#if 0 /* Maybe wrong */
  length = url_encode (login_string, temp, strlen (temp));
#else
  {
    int i;
    char c;
    
    length = strlen (temp);
    for (i = 0; i <= length; i++)
      {
        c = temp [i];
        if (c == ' ') c = '+';
        login_string [i] = c;
      }

  }
#endif
  
  sprintf (config->send_msg, LOGIN_MSG,
           config->login_page,
           config->login_server,
           config->login_server,
           config->init_page,
           length,
           login_string);

  free (login_string);
  free (temp);

#ifdef DEBUG
  if (verbose)
    {
      LOG("Sent:\n%s\n", config->send_msg);
    }
#endif

  tries = 0;
  do
    {
      sleep (2 * tries);
      result = send (config->sockfd, config->send_msg,
                     strlen (config->send_msg), 0);
    }
  while (-1 == result && tries++ < MAX_RETRIES);

  if (-1 == result)
    {
      ERROR("Send login request: %s\n", strerror(errno));
      close (config->sockfd);
      return -1;
    }

  /* Wait for a while before reading the server
   * result. Maybe we could find another way to
   * synchronize with the server ... ?
   */
  tries = 0;
  do 
    {
      sleep (2 * tries);
      result = read (config->sockfd, config->get_msg, sizeof (config->get_msg));
      /* Zero terminate the read string */
      if (result >= 0)
        config->get_msg [result] = 0;
    }
  while (-1 == result && tries++ < MAX_RETRIES);

  if (-1 == result)
    {
      ERROR("Read login reply: %s\n", strerror (errno));
      close (config->sockfd);
      return -1;
    }
#ifdef DEBUG
  if (verbose)
    {
      LOG ("Received:\n%s", config->get_msg);
    }
#endif

  close (config->sockfd);

  return 0;
}


int
internet_logout (config_data_t *config, int verbose)
{
  fd_set check_sockfd;
  struct timeval c_tv;
  int result;

  result = pre_login (config, verbose);
  if (-1 == result)
    return result;

  /* Are we logged in? */
  if (strstr (config->get_msg, config->logged_in_string) != NULL)
    {
      /* Open a connection to the login server */
      config->sockfd = open_server (config->login_server, config->server_port, verbose);
      if (config->sockfd < 0)
        {
          perror ("Failed to connect to login server");
          return -1;
        }

      /* Send logout request */
      sprintf (config->send_msg, 
               LOGOUT_MSG, 
               config->logout_page, config->login_server,
               config->init_page, config->login_server);

#ifdef DEBUG
      if (verbose)
        {
          LOG ("Sent:\n%s\n", config->send_msg);
        }
#endif

      result = send (config->sockfd, config->send_msg,
                     strlen (config->send_msg), 0);
      
      if (-1 == result)
        {
          ERROR("Send logout request: %s\n", strerror(errno));
          close (config->sockfd);
          return -1;
        }


      FD_ZERO (&check_sockfd);
      FD_SET (config->sockfd, &check_sockfd);
      
      c_tv.tv_sec  = 5;
      c_tv.tv_usec = 0;

      LOG("Trying to logout, %d second timeout ...\n", c_tv.tv_sec);

      result = select (1, &check_sockfd, NULL, NULL, &c_tv);
      if (-1 == result)
        {
          ERROR("%s does not respond, cannot log out!\n", config->logout_page);
          close (config->sockfd);
          return -1;
        }

      result = read (config->sockfd, config->get_msg, sizeof (config->get_msg));
      if (-1 == result)
        {
          ERROR("Read logout reply: %s\n", strerror (errno));
          close (config->sockfd);
          return -1;
        }

#ifdef DEBUG
      if (verbose)
        {
          LOG ("Received:\n%s", config->get_msg);
        }
#endif

      if (strstr (config->get_msg, config->logged_out_string) == NULL)
        {
          ERROR("%s: LOGOUT FAILED\n", PACKAGE_NAME);
          return -1;
        }

      LOG("%s: SUCCESSFUL LOGOUT\n", PACKAGE_NAME);
    }

  close (config->sockfd);

  return 0;
}


int
log_login (config_data_t *config, int verbose)
{
  int i, tries;
  struct tm *timep;
  time_t the_time;

  (void) time (&the_time);
  timep = localtime (&the_time);

  if (internet_login (config, verbose))
    {
      /* If login fails, try three times with increasingly
       * longer delays between each.
       */
      for (i = 0; i < MAX_RETRIES; i++)
        {
          /* Sleep for a while if it is not the first try... */
          sleep (5 * i);

          /* If we cannot get the login or csw pages we sleep some more */
          if (-1 == pre_login (config, verbose))
            {
              ERROR("%s: Failed to bring up login page.\n", PACKAGE_NAME);
              continue;
            }
          
          /* Test if we're logged in already. */
          if (strstr (config->get_msg, config->logged_in_string))
            {
              config->logged_in = 1;
              break;
            }
          
          if (!internet_login (config, verbose))
            {
              config->logged_in = 1;
              break;
            }
        }
    }

  /* Tell those who want to know why we failed. */
  if (!config->logged_in)
    {
      ERROR("%s: login failed after %d retries - aborting!\n", 
            PACKAGE_NAME, MAX_RETRIES);
    }

  return config->logged_in;
}

