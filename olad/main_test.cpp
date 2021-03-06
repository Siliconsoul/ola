/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * main_test.cpp
 * Main file for olad, parses the options, forks if required and runs the daemon
 * Copyright (C) 2005-2007 Simon Newton
 *
 */

#include <signal.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <string>

#include "ola/Logging.h"
#include "olad/DynamicPluginLoader.h"
#include "olad/olad.h"

using std::string;

// the daemon
Olad *olad;

// options struct
typedef struct {
  ola::log_level level;
  ola::log_output output;
  int daemon;
  int help;
} ola_options;


/*
 * Terminate cleanly on interrupt
 */
static void sig_interupt(int signo) {
  signo = 0;
  olad->terminate();
}

/*
 * Reload plugins
 */
static void sig_hup(int signo) {
  signo = 0;
}

/*
 * Change logging level
 *
 * need to fix race conditions here
 */
static void sig_user1(int signo) {
  signo = 0;
  ola::IncrementLogLevel();
}



/*
 * Set up the interrupt signal
 *
 * @return 0 on success, non 0 on failure
 */
static int install_signal() {
  struct sigaction act, oact;

  act.sa_handler = sig_interupt;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;

  if (sigaction(SIGINT, &act, &oact) < 0) {
    OLA_WARN << "Failed to install signal SIGINT";
    return -1;
  }

  if (sigaction(SIGTERM, &act, &oact) < 0) {
    OLA_WARN << "Failed to install signal SIGTERM";
    return -1;
  }

  act.sa_handler = sig_hup;

  if (sigaction(SIGHUP, &act, &oact) < 0) {
    OLA_WARN << "Failed to install signal SIGHUP";
    return -1;
  }

  act.sa_handler = sig_user1;

  if (sigaction(SIGUSR1, &act, &oact) < 0) {
    OLA_WARN << "Failed to install signal SIGUSR1";
    return -1;
  }
  return 0;
}


/*
 * Display the help message
 */
static void display_help() {
  printf(
"Usage: olad [--no-daemon] [--debug <level>] [--no-syslog]\n"
"\n"
"Start the ola daemon.\n"
"\n"
"  -f, --no-daemon      Don't fork into background.\n"
"  -d, --debug <level>  Set the debug level 0 .. 4 .\n"
"  -h, --help           Display this help message and exit.\n"
"  -s, --no-syslog      Log to stderr rather than syslog.\n"
"\n");
}


/*
 * Parse the command line options
 * @param argc
 * @param argv
 * @param opts  pointer to the options struct
 */
static void parse_options(int argc, char *argv[], ola_options *opts) {
  static struct option long_options[] = {
      {"no-daemon", no_argument,     0, 'f'},
      {"debug",   required_argument,   0, 'd'},
      {"help",     no_argument,   0, 'h'},
      {"no-syslog", no_argument,     0, 's'},
      {0, 0, 0, 0}
    };

  int c, ll;
  int option_index = 0;

  while (1) {
    c = getopt_long(argc, argv, "fd:hs", long_options, &option_index);
    if (c == -1)
      break;

    switch (c) {
      case 0:
        break;

      case 'f':
        opts->daemon = 0;
        break;

      case 'h':
        opts->help = 1;
        break;

      case 's':
        opts->output = ola::OLA_LOG_SYSLOG;
        break;

      case 'd':
        ll = atoi(optarg);

        switch (ll) {
          case 0:
            opts->level = ola::OLA_LOG_NONE;
            break;
          case 1:
            opts->level = ola::OLA_LOG_FATAL;
            break;
          case 2:
            opts->level = ola::OLA_LOG_WARN;
            break;
          case 3:
            opts->level = ola::OLA_LOG_INFO;
            break;
          case 4:
            opts->level = ola::OLA_LOG_DEBUG;
            break;
          default :
            break;
        }
        break;

      case '?':
        break;

      default:
       break;
    }
  }
}


/*
 * Set the default options
 *
 * @param opts  pointer to the options struct
 */
static void init_options(ola_options *opts) {
  opts->level = ola::OLA_LOG_WARN;
  opts->output = ola::OLA_LOG_STDERR;
  opts->daemon = 1;
  opts->help = 0;
}

/*
 * Run as a daemon
 *
 * Taken from apue
 */
static int daemonise() {
  pid_t pid;
  unsigned int i;
  int fd0, fd1, fd2;
  struct rlimit rl;
  struct sigaction sa;

  if (getrlimit(RLIMIT_NOFILE, &rl) < 0) {
    OLA_WARN << "Could not determine file limit";
    exit(1);
  }

  // fork
  if ((pid = fork()) < 0) {
    OLA_WARN << "Could not fork";
    exit(1);
  } else if (pid != 0) {
    exit(0);
  }

  // start a new session
  setsid();

  sa.sa_handler = SIG_IGN;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  if (sigaction(SIGHUP, &sa, NULL) < 0) {
    OLA_WARN << "Could not install signal";
    exit(1);
  }

  if ((pid= fork()) < 0) {
    OLA_WARN << "Could not fork";
    exit(1);
  } else if (pid != 0) {
    exit(0);
  }

  // close all fds
  if (rl.rlim_max == RLIM_INFINITY)
    rl.rlim_max = 1024;
  for (i = 0; i < rl.rlim_max; i++)
    close(i);

  // send stdout, in and err to /dev/null
  fd0 = open("/dev/null", O_RDWR);
  fd1 = dup(0);
  fd2 = dup(0);

  return 0;
}


/*
 * Take actions based upon the options
 */
static void handle_options(ola_options *opts) {
  if (opts->help) {
    display_help();
    exit(0);
  }

  ola::InitLogging(opts->level, opts->output);
  if (opts->daemon)
    daemonise();
}


/*
 * Parse the options, and take action
 *
 * @param argc
 * @param argv
 */
static void setup(int argc, char*argv[]) {
  ola_options opts;

  init_options(&opts);
  parse_options(argc, argv, &opts);
  handle_options(&opts);
}


/*
 * Main
 *
 */
int main(int argc, char*argv[]) {
  PluginLoader *pl;

  setup(argc, argv);

  if (install_signal())
    OLA_WARN << "Failed to install signal handlers";

  pl = new DynamicPluginLoader();
  olad = new Olad(pl);

  if (olad && olad->init() == 0) {
    olad->run();
  }
  delete olad;
  delete pl;
  return 0;
}
