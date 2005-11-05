/* main.c - GNU Alive: An auto-login & keep-alive daemon.
 *
 * Copyright (c) 2001 Jakob "kuba" Stasilowicz <kuba()unix!se>
 * Copyright (c) 2003,2004 Joachim Nilsson <joachim!nilsson()member!fsf!org>
 *
 * GNU Alive is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * GNU Alive is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <netdb.h>
#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>

#include "log.h"
#include "getopt.h"
#include "process.h"

#define USAGE printf (_("Usage: %s [-h] [-c conffile] [-p pidfile] [-loPsdvV]\n"), program_name);

static const char *program_name;

static void usage (int status);


const char *get_program_name (const char *progname)
{
#ifdef HAVE_RINDEX
  char *clean_name;

  clean_name = rindex (progname, '/');
  if (clean_name)
    return clean_name + 1;
#endif

  return progname;
}



int
main (int argc, char *argv[])
{
  extern char *optarg;
  config_data_t *config;
  char *pid_file = NULL;
  int port = -1;
  char *conf_file = NULL;
  op_t operation = NOP;
  int c;

  struct option const long_options[] = {
    {"login", no_argument, 0, 'l'},
    {"logout", no_argument, 0, 'o'},
    {"quit", no_argument, 0, 'q'},
    {"conf-file", required_argument, 0, 'c'},
    {"pid-file", required_argument, 0, 'p'},
    {"port", required_argument, 0, 'P'},
    {"status", no_argument, 0, 's'},
    {"version", no_argument, 0, 'V'},
    {"verbose", no_argument, 0, 'v'},
    {"debug", no_argument, 0, 'd'},
    {"help", no_argument, 0, 'h'},
    {"usage", no_argument, 0, 'u'},     /* Same as --help, for now */
    {NULL, 0, NULL, 0}
  };

  program_name = get_program_name (argv[0]);

  do
    {
      c = getopt_long (argc, argv, "l"  /* login */
                       "o"      /* logout */
                       "c:"     /* conf-file */
                       "p:"     /* pid-file */
                       "P:"     /* port */
                       "s"      /* status */
                       "v"      /* verbose */
                       "V"      /* version */
                       "d"      /* debug */
                       "q"      /* quit */
                       "h?",    /* help */
                       long_options, NULL);

      switch (c)
        {
        case 'l':               /* Login */
          operation = LOGIN;
          break;

        case 'c':               /* Use this config file */
          conf_file = optarg;
          break;

        case 'o':               /* Actually logout */
          operation = LOGOUT;
          break;

        case 'q':               /* Quit, don't logout */
          operation = QUIT;
          break;

        case 's':               /* Show GNU Alive status */
          operation = STATUS;
          break;

        case 'd':               /* Debug log level: ALL */
          verbose = LOG_DEBUG;

        case 'v':               /* Verbose log level */
          verbose = LOG_INFO;
          break;

        case 'V':               /* Print version. */
          printf (_("%s version %s\n"), PACKAGE_NAME, PACKAGE_VERSION);
          exit (EXIT_SUCCESS);
          break;

        case 'p':
          pid_file = optarg;
          break;

        case 'P':
          port = atoi (optarg);
          break;

        case '?':
        case 'h':
          usage (EXIT_SUCCESS);
          break;

        case 'u':
          USAGE;
          exit (EXIT_SUCCESS);
          break;

        case EOF:
          break;

        default:
          ERR(_("Unrecognised option, %s"), argv[0]);
          usage (EXIT_FAILURE);
        }
    }
  while (EOF != c);

  /* Read /etc/qadsl.conf or similar */
  if (NOP == operation)
    {
      usage (EXIT_FAILURE);
    }

  config = config_load (conf_file);
  if (!config)
    {
      exit (EXIT_FAILURE);
    }
  /* Override qadsl.conf and default values when given on command line. */
  if (pid_file)   config->pid_file = pid_file;
  if (port != -1) config->server_port = port;

  /* Handle request */
  return process (config, operation);
}


static void
usage (int status)
{
  printf (_("%s - Auto-login & keep-alive for Internet connections.\n\n"), PACKAGE_NAME);

  USAGE;

  printf (_("\
Options:\n\
-l, --login               Send login request.\n\
-q, --quit                Quit daemon, don't logout.\n\
-o, --logout              Send logout request.\n\
-c, --conf-file=FILE      Use settings from FILE instead of " GLOBAL_CONF ".\n\
-p, --pid-file=FILE       Use FILE to store PID, default is " PID_FILE ".\n\
-P, --port=PORT           Connect to PORT on server, default is " PORT ".\n\
-s, --status              Display status of login daemon.\n\
-v, --verbose             Display more information, on screen and in logfile.\n\
-d, --debug               Display even more information, huge amounts!\n\
-V, --version             Display version information and exit.\n\
-h, --help                Display this help and exit.\n\
    --usage               Give a short usage message.\n\
\n\
Report bugs to <" PACKAGE_BUGREPORT ">\n\
"));

  exit (status);
}


/* Local Variables:
 * mode: C;
 * c-file-style: gnu;
 * indent-tabs-mode: nil;
 * End:
 */
