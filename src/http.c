/* http.c - Handles the communication with the login server.
 *
 * Copyright (c) 2003,2004 Joachim Nilsson <joachim!nilsson()member!fsf!org>

 * http_open_server()     - Open and connect to a server on a specified port.
 * http_pre_login()       - Bring up Orbyte login screen.
 * http_internet_login()  - Handles the login phase.
 * http_internet_logout() - Handles the graceful logout for the client.
 * http_do_login()        - Handles periodic logins for the daemon only.
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

#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "http.h"
#include "log.h"


/**
 * http_open_server - Open and connect to a server on a specified port.
 * @name:    Connect to this named server.
 * @port:    Connect to this port.
 * @verbose: Diagnostic output.
 *
 * This is a local helper function for the rest of the handlers in this
 * class. It opens a connection to a specified server, @name, which can
 * be a proper Internet name or IP address, on a specified @port.
 *
 * Returns: -1 and errno on error, otherwise
 *          socket descriptior.
 */

static int 
http_open_server (char *name, short port, int verbose)
{
  int result, sockfd;
  struct hostent *he;
  struct sockaddr_in address;
  

  /* Try to find the login host */
  LOG("DNS lookup - ");

  he = gethostbyname (name);
  if (he == NULL)
    {
      ERROR("cannot find the IP# of the login server (%s): %s\n", 
	    name, strerror (errno));
      result = -1;
    }
  else
    {
      /* Setup a socket */
      LOG("%s=0x%X\n", he->h_name, he->h_addr);

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
              LOG("Connecting to login server: %s(0x%X)\n", name, (int)he->h_addr);
            }
          result = connect (sockfd, (struct sockaddr *) &address, sizeof (address));
          if (-1 == result)
            {
              close (sockfd);
              ERROR("Failed to connect to login server (%s): %s\n",
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
 * http_pre_login - Bring up Orbyte login screen.
 * @config:  Pointer to configuration data.
 * @sockfd: Socket we are connected to.
 * @verbose: Diagnostic output.
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
http_pre_login (config_data_t *config, int verbose)
{
  int result, tries;

  /* Open a connection to the login server */
  config->sockfd = http_open_server (config->login_server, config->server_port, verbose);
  if (config->sockfd < 0)
    {
      ERROR ("Failed to connect to login server (%s): %s\n",
             config->login_server, strerror (errno));
      return -1;
    }

  /* Register and get login page */
  sprintf (config->send_msg, PRELOGIN_MSG,
           config->init_page,
           config->login_server,
           config->login_server);

#ifdef DEBUG
  LOG ("Sent:\n%s\n", config->send_msg);
#endif

  result = send (config->sockfd, config->send_msg, strlen (config->send_msg), 0);

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
      LOG ("Received:\n%s", config->get_msg);
#endif
      result = 0;
    }

  close (config->sockfd);

  return result;
}


/**
 * http_internet_login - Handles the login phase.
 * @config:  Pointer to configuration data.
 * @sockfd:  Socket we are connected to.
 * @verbose: Diagnostic output.
 * 
 * This is the function that handles the logging in to the
 * Internet gateway of the ISP.
 *
 * Returns: 0 if login was successful, otherwise -1.
 */

int
http_internet_login (config_data_t *config, int verbose)
{
  int result, tries, length;
  char *login_string, *temp;

  /* Open a connection to the login server */
  config->sockfd = http_open_server (config->login_server, config->server_port, verbose);
  if (config->sockfd < 0)
    {
      ERROR("Failed to connect to login server (%s): %s\n",
            config->login_server, strerror (errno));

      return -1;
    }

  /* Compose the login string. */
  length = (config->login_string_header ? strlen (config->login_string_header) : 0)
    + strlen (config->username_key) + strlen (config->username)
    + strlen (config->password_key) + strlen (config->password) 
    + strlen (config->login_string_footer) + strlen ("Plus approximately 10%");

  temp = (char *) malloc (length);
  if (!temp)
    {
      close (config->sockfd);
      ERROR ("Failed to allocate memory (%d bytes): %s\n",
             length, strerror (errno));

      return -1;
    }

  result = 0;
  if (config->login_string_header)
    {
      result = sprintf (temp, "%s&", config->login_string_header);
    }
  
  result += sprintf (&temp[result], "%s=%s&%s=%s",
                     config->username_key,
                     config->username,
                     config->password_key,
                     config->password);

  if (config->login_string_footer)
    {
      result += sprintf (&temp[result], "&%s", config->login_string_footer);
    }
  
#ifdef DEBUG
  LOG("Non-url-encoded login string:\n%s\n", temp);
#endif

  login_string = (char *) malloc (length);
  if (!login_string)
    {
      free (temp);
      close (config->sockfd);
      ERROR ("Failed to allocate memory (%d bytes): %s\n",
             length, strerror (errno));

      return -1;
    }

  /* Encode the the login string. No spaces! */
  {
    int i;

    length = strlen (temp);
    for (i = 0; i < length; i++)
      {
	if (' ' == temp [i])
	  login_string [i] = '+';
	else
	  login_string [i] = temp [i];
      }
  }

  /* Paste it all into the login template */
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
  LOG("Sent:\n%s\n", config->send_msg);
#endif

  /* Send login string to server */
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
      close (config->sockfd);
      ERROR("Failed to send login request to login server (%s): %s\n", 
	    config->login_server, strerror(errno));

      return -1;
    }

  /* Wait for a while before reading the server result. Maybe we could find
   * another way to synchronize with the server ... ?
   */
  tries = 0;
  do 
    {
      sleep (2 * tries);
      result = read (config->sockfd, config->get_msg, sizeof (config->get_msg));
      /* Zero terminate the read string */
      if (result >= 0)
	{
	  config->get_msg [result] = 0;
	}
    }
  while (-1 == result && tries++ < MAX_RETRIES);

  if (-1 == result)
    {
      close (config->sockfd);
      ERROR("Read login reply: %s\n", strerror (errno));

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


/**
 * http_internet_logout - Handles the graceful logout for the client.
 * @config:  Pointer to configuration data.
 * @verbose: Diagnostic output.
 *
 * This function is called by the client to gracefully logout from
 * the login server.
 *
 * Returns: 0 when logged out OK, otherwise -1.
 */

int
http_internet_logout (config_data_t *config, int verbose)
{
  fd_set check_sockfd;
  struct timeval c_tv;
  int result;

  result = http_pre_login (config, verbose);
  if (-1 == result)
    {
      return result;
    }

  /* Are we logged in? */
  if (http_logged_in_already (config))
    {
      /* Open a connection to the login server */
      config->sockfd = http_open_server (config->login_server, config->server_port, verbose);
      if (config->sockfd < 0)
        {
          ERROR("Failed to connect to login server (%s): %s\n",
		config->login_server, strerror (errno));

          return -1;
        }

      /* Send logout request */
      sprintf (config->send_msg, 
               LOGOUT_MSG, 
               config->logout_page, config->login_server,
               config->init_page, config->login_server);

#ifdef DEBUG
      LOG ("Sent:\n%s\n", config->send_msg);
#endif

      result = send (config->sockfd, config->send_msg,
                     strlen (config->send_msg), 0);
      
      if (-1 == result)
        {
          close (config->sockfd);
          ERROR("Logout request failed: %s\n", strerror(errno));

          return -1;
        }

      FD_ZERO (&check_sockfd);
      FD_SET (config->sockfd, &check_sockfd);
      
      c_tv.tv_sec  = 5;
      c_tv.tv_usec = 0;

      LOG("Trying to logout, %d second timeout ...\n", (int)c_tv.tv_sec);

      result = select (1, &check_sockfd, NULL, NULL, &c_tv);
      if (-1 == result)
        {
          close (config->sockfd);
          ERROR("%s does not respond, cannot log out!\n", config->logout_page);

          return -1;
        }

      result = read (config->sockfd, config->get_msg, sizeof (config->get_msg));
      if (-1 == result)
        {
          close (config->sockfd);
          ERROR("Read logout reply: %s\n", strerror (errno));

          return -1;
        }

#ifdef DEBUG
      LOG ("Received:\n%s", config->get_msg);
#endif

      /* Interpret answer from server. */
      if (http_test_if_logged_out (config))
        {
          close (config->sockfd);
          ERROR("%s: LOGOUT FAILED\n", PACKAGE_NAME);

          return -1;
        }

      LOG("%s: SUCCESSFUL LOGOUT\n", PACKAGE_NAME);
    }

  /* Make sure to close the connection before leaving. */
  close (config->sockfd);

  return 0;
}


/** 
 * http_do_login - Handles periodic logins for the daemon only
 * @config:  Pointer to configuration data.
 * @verbose: Diagnostic output.
 *
 * Ths function is exclusively called by the daemon but placed here to keep
 * all similar functionality in one place. It takes care to 
 * 
 *
 * 
 * Returns: Value of config->logged_in.
 */

int
http_do_login (config_data_t *config, int verbose)
{
  int try, result;

  /* XXX - I've tried to redo this function as a do-while() loop instead
   * of this brain dead if-for()-break-continue mess. But it turned out
   * that the actual ordering of events to the server got screwed up if
   * http_pre_login() fails. We always need to get the login page up 
   * first before we try the actual login otherwise the server will be
   * very crossed with us.
   */
  result = http_internet_login (config, verbose);
  /* If we fail to do the hokey pokey */
  if (result || http_test_if_logged_in (config))
    {
      LOG (__FUNCTION__ "(): failed first login attempt.\n");

      /* Login failed, or status unknown. */
      config->logged_in = 0;

      /* When login fails, retry two more times with increasingly
       * longer delays between each try.
       */
      for (try = 1; try < MAX_RETRIES && result; try++)
        {
          /* Sleep for a while if it is not the first try... */
          LOG (__FUNCTION__ "(): waiting a while before retrying...\n");
          sleep (5 * i);

          /* If we cannot get the login or csw pages we sleep some more */
          result = http_pre_login (config, verbose);
          if (result)
            {
              ERROR (__FUNCTION__ "(): failed to bring up login page.\n");
            }
          else
            {
              /* Okey dokey - pre-login phase OK. Now, see what page we 
               * actually received: login page or CSW/Session Control Window/
               * After each successful attempt to get the login page we
               * also need to check if that page redirected us to the
               * post login page - if so there is no need to actually
               * do the login tango with the server.
               */
              result = http_test_if_logged_in (config);
              if (result)
                {
                  /* Actual login page - do the login */
                  result = http_internet_login (config, verbose);

                  /* For the logfile -- tell status. */
                  if (result)
                    {
                      LOG (__FUNCTION__ "(): failed to send login (attempt #%d).\n", try + 1);
                    }
                  else
                    {
                      result = http_internet_login (config, verbose);
                      if (result)
                        {
                          LOG (__FUNCTION__ "(): login sent successfully but the reply was unexpected.\n");
#ifdef DEBUG
                          LOG (__FUNCTION__ "(): Here is the reply:\n");
                          LOG (config->get_msg);
#else
                          LOG (__FUNCTION__ "(): To spare your logfile the actual reply is only visible in debug version.\n");
                          LOG (__FUNCTION__ "(): Rebuild with --enable-debug option to configure script.\n");
#endif
                        }
                      else
                        {
                          LOG (__FUNCTION__ "(): OK!\n");
                        }
                    }
                }
            }
        }
    }

  /* Check if we managed to login and in case we failed,
   * tell those who want to know why we failed. 
   */
  if (result)
    {
      ERROR (__FUNCTION__ "(): failed %d retries - backing off for a while.\n",
             MAX_RETRIES);
    }

  return config->logged_in;
}

