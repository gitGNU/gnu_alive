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

#include <alloca.h>
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
  he = gethostbyname (name);
  if (he == NULL)
    {
      ERROR ("DNS lookup of login server %s failed to resolve to an IP#", name);
      ERROR ("Try the commands nslookup, host or dig for more details.");
      ERROR ("gethostbyname() returns: %s", strerror (errno));
      result = -1;
    }
  else
    {
      /* Setup a socket */
      sockfd = socket (PF_INET, SOCK_STREAM, 0);
      if (-1 == sockfd)
        {
          ERROR ("Failed to create a socket: %s", strerror (errno));
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
          result = connect (sockfd, (struct sockaddr *) &address, sizeof (address));
          if (-1 == result)
            {
              close (sockfd);
              ERROR ("Failed to connect to login server (%s): %s",
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

static int __http_pre_login (config_data_t *config, int verbose)
{
  int result, tries;

  /* Clear the read string to make sure we don't rely on previous results. */
  config->get_msg [0] = 0;

  /* Open a connection to the login server */
  LOG ("Connecting to login server, %s:%d.", config->login_server, config->server_port);
  config->sockfd = http_open_server (config->login_server, config->server_port, verbose);
  if (config->sockfd < 0)
    {
      ERROR ("Failed to connect to login server (%s): %s", config->login_server, strerror (errno));
      config->logged_in = 0;
      return -1;
    }

  /* Register and get login page */
  sprintf (config->send_msg, PRELOGIN_MSG,
           config->init_page,
           config->login_server,
           config->login_server);

  DEBUG (config->send_msg);

  result = send (config->sockfd, config->send_msg, strlen (config->send_msg), 0);
  if (-1 == result)
    {
      ERROR ("Failed sending request for %s.", config->init_page);
      ERROR ("Closing connection to login server.");
      close (config->sockfd);
      config->logged_in = 0;

      return result;
    }

  LOG ("Waiting for reply from server...");
  tries = 0;
  do
    {
      sleep (2 * tries);
      result = read (config->sockfd, config->get_msg, MAXDATASIZE);
    }
  while (-1 == result && tries++ < MAX_RETRIES);

  if (-1 == result)
    {
      ERROR("Failed to \"ping\" login server: %s", strerror (errno));
      config->logged_in = 0;
    }
  else
    {
      /* Zero terminate the read string */
      config->get_msg [result] = 0;

      /* OK */
      result = 0;
    }

  close (config->sockfd);

  return result;
}

int http_pre_login (config_data_t *config, int verbose)
{
  int result;

  result = __http_pre_login (config, verbose);
  http_test_if_logged_in (config, verbose);
  if ((result && verbose) || (config->logged_in == 0))
    {
      LOG ("%s(): Login server \"pinged\": not logged in...", __FUNCTION__);
      if (IS_DEBUG (verbose))
        {
          DEBUG ("%s(): Here is the reply:\n%s", __FUNCTION__, config->get_msg);
        }
      else
        {
          LOG ("%s(): Start %s with --verbose --debug to see whole reply.", __FUNCTION__, PACKAGE_NAME);
        }
    }

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

static int __http_internet_login (config_data_t *config, int verbose)
{
  int result = 0, tries, length;
  char *login_string, *temp;

  /* Clear the read string to make sure we don't rely on previous results. */
  config->get_msg [0] = 0;
  config->logged_in   = 0;

  /* Open a connection to the login server */
  LOG ("Connecting to login server, %s:%d.", config->login_server, config->server_port);
  config->sockfd = http_open_server (config->login_server, config->server_port, verbose);
  if (config->sockfd < 0)
    {
      ERROR("Failed to connect to login server (%s): %s",
            config->login_server, strerror (errno));
      return -1;
    }

  /* Compose the login string. */
  DEBUG ("Composing non-url-encoded login string.");
  length = (config->login_string_header ? strlen (config->login_string_header) : 0)
    + strlen (config->username_key) + strlen (config->username)
    + strlen (config->password_key) + strlen (config->password)
    + strlen (config->login_string_footer) + strlen ("Plus approximately 10%");

  temp = (char *) alloca (length);
  if (!temp)
    {
      close (config->sockfd);
      ERROR ("Failed to allocate memory (%d bytes): %s",
             length, strerror (errno));
      return -1;
    }

  if (config->login_string_header) result = sprintf (temp, "%s&", config->login_string_header);

  result += sprintf (&temp[result], "%s=%s&%s=%s",
                     config->username_key,
                     config->username,
                     config->password_key,
                     config->password);

  if (config->login_string_footer) result += sprintf (&temp[result], "&%s", config->login_string_footer);

  DEBUG ("Login string: %s", temp);

  login_string = (char *) alloca (length);
  if (!login_string)
    {
      close (config->sockfd);
      ERROR ("Failed to allocate memory (%d bytes): %s", length, strerror (errno));
      return -1;
    }

  /* Encode the the login string. No spaces! */
  {
    int i;

    length = strlen (temp) + 1;
    for (i = 0; i < length; i++)
      {
        if (' ' == temp [i]) login_string [i] = '+';
        else                 login_string [i] = temp [i];
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

  /* Send login string to server */
  DEBUG (config->send_msg);

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
      ERROR("Failed to send login request to server (%s): %s",
            config->login_server, strerror(errno));
      return -1;
    }

  /* Wait for a while before reading the server result. Maybe we could find
   * another way to synchronize with the server ... ?
   */
  LOG ("Waiting for reply from server...");
  tries = 0;
  do
    {
      sleep (2 * tries);
      result = read (config->sockfd, config->get_msg, MAXDATASIZE);
    }
  while (-1 == result && tries++ < MAX_RETRIES);

  /* Make sure to close the connection as soon as we're done. */
  close (config->sockfd);

  if (-1 == result)
    {
      ERROR("%s(): Error reading login reply: %s", __FUNCTION__, strerror (errno));

      /* Clear the read string to avoid confusion. */
      config->get_msg [0] = 0;
    }
  else
    {
      /* Zero terminate the read string. */
      config->get_msg [result] = 0;

      /* OK - we've done our part.  The caller must now check the reply. */
      result = 0;
    }

  return result;
}

int http_internet_login (config_data_t *config, int verbose)
{
  int result;

  result = __http_internet_login (config, verbose);
  http_test_if_logged_in (config, verbose);

  if ((result && verbose) || (config->logged_in == 0))
    {
      LOG ("%s(): login failed.", __FUNCTION__);
      if (IS_DEBUG (verbose))
        {
          DEBUG ("%s(): Here is the reply:\n%s", __FUNCTION__, config->get_msg);
        }
      else
        {
          LOG ("%s(): Start %s with --verbose --debug to see whole reply.", __FUNCTION__, PACKAGE_NAME);
        }
    }

  return result;
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

int __http_internet_logout (config_data_t *config, int verbose)
{
  fd_set check_sockfd;
  struct timeval c_tv;
  int result;

  /* Clear the read string to make sure we don't rely on previous results. */
  config->get_msg [0] = 0;

  /* Open a connection to the login server */
  LOG ("Connecting to login server, %s:%d.", config->login_server, config->server_port);
  config->sockfd = http_open_server (config->login_server, config->server_port, verbose);
  if (config->sockfd < 0)
    {
      ERROR("Failed to connect to login server (%s): %s",
            config->login_server, strerror (errno));
      config->logged_in = 0;

      return -1;
    }

  /* Send logout request */
  sprintf (config->send_msg,
           LOGOUT_MSG,
           config->logout_page, config->login_server,
           config->init_page, config->login_server);

  DEBUG (config->send_msg);

  result = send (config->sockfd, config->send_msg,
                 strlen (config->send_msg), 0);

  if (-1 == result)
    {
      close (config->sockfd);
      ERROR("Logout request failed: %s", strerror(errno));
      config->logged_in = 0;

      return -1;
    }

  FD_ZERO (&check_sockfd);
  FD_SET (config->sockfd, &check_sockfd);

  c_tv.tv_sec  = 5;
  c_tv.tv_usec = 0;

  LOG ("Waiting for logout reply, %d second timeout ...", (int)c_tv.tv_sec);
  /* XXX - Use poll() instead! */
  result = select (1, &check_sockfd, NULL, NULL, &c_tv);
  if (-1 == result)
    {
      close (config->sockfd);
      ERROR ("%s does not respond, cannot log out!", config->logout_page);
      config->logged_in = 0;

      return -1;
    }

  DEBUG ("Poll says a reply has arrived, reading reply...");
  result = read (config->sockfd, config->get_msg, MAXDATASIZE);
  if (-1 == result)
    {
      close (config->sockfd);
      ERROR ("Read logout reply: %s", strerror (errno));
      config->logged_in = 0;

      return -1;
    }

  DEBUG ("Received:\n%s", config->get_msg);

  /* Interpret answer from server. */
  if (http_test_if_logged_out (config))
    {
      close (config->sockfd);
      ERROR("LOGOUT FAILED");
      config->logged_in = 0;

      return -1;
    }

  LOG ("SUCCESSFUL LOGOUT");

  /* Make sure to close the connection before leaving. */
  close (config->sockfd);

  return 0;
}

int
http_internet_logout (config_data_t *config, int verbose)
{
  int result;

  result = http_internet_logout (config, verbose);

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
  if (result)
    {
      ERROR ("%s(): failed first login attempt.", __FUNCTION__);

      /* Login failed, or status unknown. */
      config->logged_in = 0;

      /* When login fails, retry two more times with increasingly
       * longer delays between each try.
       */
      for (try = 1; try < MAX_RETRIES && result; try++)
        {
          /* Sleep for a while if it is not the first try... */
          LOG ("%s(): waiting a while before retrying...", __FUNCTION__);
          sleep (5 * try);

          /* If we cannot get the login or csw pages we sleep some more */
          result = http_pre_login (config, verbose);
          if (result)
            {
              DEBUG ("%s(): failed to bring up login page.", __FUNCTION__);
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
              result = http_test_if_logged_in (config, verbose);
              if (result)
                {
                  /* Actual login page - do the login */
                  result = http_internet_login (config, verbose);

                  /* For the logfile -- tell status. */
                  if (result)
                    {
                      LOG ("%s(): failed to send login (attempt #%d).",
                           __FUNCTION__, try + 1);
                    }
                  else
                    {
                      DEBUG ("%s(): Success.", __FUNCTION__);
                      break;
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
      ERROR ("%s(): failed %d retries - backing off for a while.",
             __FUNCTION__, MAX_RETRIES);
    }
  else
    {
      DEBUG ("%s(): Success.", __FUNCTION__);
    }

  return config->logged_in;
}

/* Local Variables:
 * mode: C;
 * c-file-style: gnu;
 * indent-tabs-mode: nil;
 * End:
 */
