/* qADSL - An automatic login daemon for WAN/LAN/ADSL connections
 *
 * qADSL is an auto-login/connection-keep-alive daemon for various
 * WAN/LAN/ADSL connections provided by several (Swedish) ISPs.
 * To mention a few: Telia ADSL, COMHEM, Tiscali. All of which are
 * based on the commercial product "Orbyte Wireless System" from the
 * Swedish company Åkerströms nowire AB - see <http://www.nowire.se>
 *
 * Copyright (c) 2001 Jakob "kuba" Stasilowicz <kuba()unix!se>
 * Copyright (c) 2003,2004 Joachim Nilsson <joachim!nilsson()member!fsf!org>
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
#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>

#include "getopt.h"
#include "process.h"


static void usage (int status);

char *program_name;


int
main (int argc, char *argv[])
{
  extern char *optarg;
  config_data_t *config;
  char *pid_file  = NULL;
  char *conf_file = NULL;
  int verbose = 0;
  op_t operation = NOP;
  char c;

  struct option const long_options[] =
    {
      {"login", no_argument, 0, 'l'},
      {"logout", no_argument, 0, 'o'},
      {"conf-file", required_argument, 0, 'c'},
      {"pid-file", required_argument, 0, 'p'},
      {"status", no_argument, 0, 's'},
      {"version", no_argument, 0, 'V'},
      {"verbose", no_argument, 0, 'v'},
      {"help", no_argument, 0, 'h'},
      {NULL, 0, NULL, 0}
    };

#ifdef HAVE_RINDEX
  program_name = rindex (argv[0], '/');
  if (program_name) 
    program_name ++;
  else
#endif
    program_name = argv[0];

  while ((c = getopt_long (argc, argv,
			   "l" 	/* login */
			   "o"	/* logout */
			   "c:"	/* conf-file */
			   "p:"	/* pid-file */
			   "s"	/* status */
			   "v"	/* verbose */
			   "V"	/* version */
			   "h?", /* help */
			   long_options, (int *)0)) != EOF)
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

        case 'v':		/* Set verbosity to high */
          verbose = 2; 		/* 2 is for client side verbosity */
          break;

        case 'V':		/* Print version. */
          printf ("%s version %s\n", PACKAGE_NAME, PACKAGE_VERSION);
	  exit (EXIT_SUCCESS);
          break;

        case 'p':
          pid_file = optarg;
          break;

        case '?':
        case 'h':
        default:
	  usage (EXIT_SUCCESS);
          break;
        }
    }

  /* Read /etc/qadsl.conf or similar */
  if (operation == NOP) 
    {
      usage (EXIT_FAILURE);
    }

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

static void
usage (int status)
{
  printf ("%s - Auto-login & keep-alive for Internet connections.\n\n", 
	  program_name);
  printf ("Usage: %s [-h] [-c conffile] [-p pidfile] [-losvV]\n", 
	  program_name);
  printf ("\
Options:\n\
-l, --login               Try to login\n\
-o, --logout              Try to logout\n\
-c, --conf-file=FILE      Use settings from FILE instead of " GLOBAL_CONF "\n\
-p, --pid-file=FILE       Use FILE to store PID\n\
-s, --status              Display status of qadsl daemon\n\
-v, --verbose             Display more information, on screen and in logfile\n\
-V, --version             Display version information and exit\n\
-h, --help                Display this help and exist\n\
\n\
Report bugs to <" PACKAGE_BUGREPORT ">\n\
");

  exit (status);
}
