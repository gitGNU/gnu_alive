/* qADSL - An automatic login daemon for WAN/LAN/ADSL connections
 *
 * qADSL is an auto-login/connection-keep-alive daemon for various
 * WAN/LAN/ADSL connections provided by several (Swedish) ISPs.
 * To mention a few: Telia ADSL, COMHEM, Tiscali. All of which are
 * based on the commercial product "Orbyte Wireless System" from the
 * Swedish company Åkerströms nowire AB - see <http://www.nowire.se>
 *
 * Copyright (c) 2001 Jakob "kuba" Stasilowicz <kuba()unix!se>
 * Copyright (c) 2003 Joachim Nilsson <joachim()vmlinux!org>
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

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "config.h"
#include "process.h"


#define USAGE(name)                                                     \
  "Usage: %s [-h] [-c file] [-lovV]\n"                                  \
  "       -h       Print this message\n"                                \
  "       -c file  Use settings from file instead of " GLOBAL_CONF "\n"	\
  "       -l       Try to login\n"                                      \
  "       -o       Try to logout\n"                                     \
  "       -p file  Use this PID file\n"                                 \
  "       -V       Be verbose.\n", name

int
main (int argc, char *argv[])
{
  extern char *optarg;
  config_data_t *config;
  char *pid_file  = NULL;
  char *progname  = argv[0];
  char *conf_file = NULL;
  int verbose = 0;
  op_t operation = NOP;
  char c;

  /*   printf ("%s %s - http://savannah.gnu.org/projects/qadsl/\n", */
  /* 	  PACKAGE_NAME, PACKAGE_VERSION); */

  /* XXX - Find out if there is an instance of qadsl running already -> pid-file/lock-file
   *   if threre is && no option -> presume logout.
   *   else && no option -> presume login
   */

  /* getopt() usually works on GNU and BSD systems,
   * getopt_long() is not that common.
   */
  while ((c = getopt (argc, argv, "lc:ovVp:h?")) != -1)
    {
      switch (c)
        {
        case 'l':		/* Login */
          operation = LOGIN;
          break;

        case 'c':		/* Use this config file */
          conf_file = optarg;
          break;

        case 'o':		/* Logout */
          operation = LOGOUT;
          break;

        case 's':		/* Show qADSL status */
          operation = STATUS;
          break;

        case 'v':		/* Print version. */
          printf ("%s version %s\n", PACKAGE_NAME, PACKAGE_VERSION);
          break;

        case 'V':		/* Set verbosity to high */
          verbose = 2; 		/* 2 is for client side verbosity */
          break;

        case 'p':
          pid_file = optarg;
          break;

        case '?':
        case 'h':
        default:
          printf (USAGE((progname)));
          exit (EXIT_SUCCESS);
          break;
        }
    }

  /* Read /etc/qadsl.conf or similar */
  config = config_load (conf_file);
  if (!config)
    {
      exit (EXIT_FAILURE);
    }

  /* Override any qadsl.conf settings or configure --with-pidfile
   * if the user specified a different PID file on the command line.
   */
  if (pid_file)
    {
      config->pid_file = pid_file;
    }

  /* Handle request */
  return process (config, operation, verbose);
}
