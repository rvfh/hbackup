/*
     Copyright (C) 2006  Herve Fache

     This program is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License version 2 as
     published by the Free Software Foundation.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place - Suite 330,
     Boston, MA 02111-1307, USA.
*/

using namespace std;

#include <iostream>
#include <fstream>
#include <string>
#include <signal.h>
#include <errno.h>

#include "params.h"
#include "list.h"
#include "db.h"
#include "tools.h"
#include "metadata.h"
#include "common.h"
#include "filters.h"
#include "parsers.h"
#include "filelist.h"
#include "cvs_parser.h"
#include "clients.h"
#include "hbackup.h"
#include "version.h"

/* DEFAULTS */

/* Verbosity */
static int verbose = 0;

/* Configuration path */
static string default_config_path = "/etc/hbackup.conf";

/* Configuration path */
static string default_db_path = "/hbackup";


/* Signal received? */
static int killed = 0;

static void show_version(void) {
  printf("(c) 2006-2007 Hervé Fache, version %u.%u", VERSION_MAJOR, VERSION_MINOR);
  if (VERSION_BUGFIX != 0) {
    printf(".%u", VERSION_BUGFIX);
  }
  if (BUILD != 0) {
    printf(" (build %u)", BUILD);
  }
  printf("\n");
}

static void show_help(void) {
  show_version();
  printf("Options are:\n");
  printf(" -h or --help     to print this help and exit\n");
  printf(" -v or --version  to print version and exit\n");
  printf(" -c or --config   to specify a configuration file other than \
/etc/hbackup.conf\n");
  printf(" -s or --scan     to scan the database for missing data\n");
  printf(" -t or --check    to check the database for corrupted data\n");
}

int verbosity(void) {
  return verbose;
}

int terminating(void) {
  return killed;
}

void sighandler(int signal) {
  fprintf(stderr, "Received signal, aborting...\n");
  killed = signal;
}

int main(int argc, char **argv) {
  string            config_path       = "";
  string            db_path           = "";
  int               failed            = 0;
  int               argn              = 0;
  bool              scan              = false;
  bool              check             = false;
  bool              configcheck       = false;
  bool              expect_configpath = false;
  struct sigaction  action;

  /* Set signal catcher */
  action.sa_handler = sighandler;
  sigemptyset (&action.sa_mask);
  action.sa_flags = 0;
  sigaction (SIGINT, &action, NULL);
  sigaction (SIGHUP, &action, NULL);
  sigaction (SIGTERM, &action, NULL);

  /* Analyse arguments */
  while (++argn < argc) {
    char letter = ' ';

    /* Get config path if request */
    if (expect_configpath) {
      config_path       = argv[argn];
      expect_configpath = false;
    }

    /* -* */
    if (argv[argn][0] == '-') {
      /* --* */
      if (argv[argn][1] == '-') {
        if (! strcmp(&argv[argn][2], "config")) {
          letter = 'c';
        } else if (! strcmp(&argv[argn][2], "help")) {
          letter = 'h';
        } else if (! strcmp(&argv[argn][2], "restore")) {
          letter = 'r';
        } else if (! strcmp(&argv[argn][2], "scan")) {
          letter = 's';
        } else if (! strcmp(&argv[argn][2], "check")) {
          letter = 't';
        } else if (! strcmp(&argv[argn][2], "configcheck")) {
          letter = 'u';
        } else if (! strcmp(&argv[argn][2], "verbose")) {
          letter = 'v';
        } else if (! strcmp(&argv[argn][2], "version")) {
          letter = 'V';
        }
      } else if (argv[argn][1] == 'v') {
        letter = argv[argn][1];
        if (argv[argn][2] == 'v') {
          verbose++;
          if (argv[argn][3] == 'v') {
            verbose++;
            if (argv[argn][4] != '\0') {
              letter = ' ';
            }
          } else if (argv[argn][3] != '\0') {
            letter = ' ';
          }
        } else if (argv[argn][2] != '\0') {
          letter = ' ';
        }
      } else if (argv[argn][2] == '\0') {
        letter = argv[argn][1];
      }

      switch (letter) {
        case 'c':
          expect_configpath = true;
          break;
        case 'h':
          show_help();
          return 0;
        case 's':
          scan = true;
          break;
        case 't':
          check = true;
          break;
        case 'u':
          configcheck = true;
          break;
        case 'v':
          verbose++;
          break;
        case 'V':
          show_version();
          return 0;
        default:
          fprintf(stderr, "Unrecognised option: %s\n", argv[1]);
          return 2;
      }
    }
  }

  if (expect_configpath) {
    fprintf(stderr, "Missing config path\n");
    return 2;
  }

  if (config_path == "") {
    config_path = default_config_path;
  }

  /* Open configuration file */
  ifstream config_file(config_path.c_str());

  if (! config_file.is_open()) {
    cerr << "Configuration file not found " << config_path << endl;
    failed = 2;
  } else {
    /* Read configuration file */
    Clients clients;
    Client  *client = NULL;
    string  buffer;
    int     line    = 0;

    if (verbosity() > 1) {
      printf(" -> Reading configuration file\n");
    }

    while (! config_file.eof() && ! failed) {
      getline(config_file, buffer);
      char    keyword[256];
      char    type[256];
      string  value;
      int     params = params_readline(buffer, keyword, type, &value);

      line++;
      if (params > 1) {
        if (! strcmp(keyword, "db")) {
          db_path = value;
        } else if (! strcmp(keyword, "client")) {
          client = new Client(value);

          clients.push_back(client);
        } else if (client != NULL) {
          if (! strcmp(keyword, "hostname")) {
            client->setHostOrIp(value);
          } else
          if (! strcmp(keyword, "protocol")) {
            client->setProtocol(value);
          } else
          if (! strcmp(keyword, "username")) {
            client->setUsername(value);
          } else
          if (! strcmp(keyword, "password")) {
            client->setPassword(value);
          } else
          if (! strcmp(keyword, "listfile")) {
            client->setListfile(value);
          } else {
            cerr << "Unrecognised keyword '" << keyword
              << "' in configuration file, line " << line
              << endl;
            failed = 2;
          }
        } else {
          cerr << "Unrecognised keyword in configuration file, line " << line
            << endl;
          failed = 2;
        }
      } else if (params < 0) {
        errno = EUCLEAN;
        cerr << "Syntax error in configuration file, line " << line << endl;
        failed = 2;
      }
    }
    config_file.close();

    if (! failed && ! configcheck) {
      if (db_path == "") {
        db_path = default_db_path;
      }
      /* Open backup database */
      if (check) {
        db_check(db_path, "");
      } else
      if (scan) {
        db_scan(db_path, "");
      } else
      if (db_open(db_path) == 2) {
        cerr << "Failed to open database in '" << db_path << "'" << endl;
        failed = 2;
      } else {
        string mount_path = db_path + "/mount";

        /* Make sure we have a mount path */
        if (testdir(mount_path, 1) == 2) {
          cerr << "Failed to create mount point" << endl;
          failed = 2;
        } else

        /* Backup */
        if (clients.backup(mount_path, configcheck)) {
          failed = 1;
        }
        db_close();
      }
    }
  }
  return failed;
}
