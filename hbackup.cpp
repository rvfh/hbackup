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
#include <vector>
#include <signal.h>
#include <errno.h>

#include "list.h"
#include "files.h"
#include "db.h"
#include "filters.h"
#include "parser.h"
#include "parsers.h"
#include "cvs_parser.h"
#include "paths.h"
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
      unsigned int pos = buffer.find("\r");
      if (pos != string::npos) {
        buffer.erase(pos);
      }
      vector<string> params;

      line++;
      if (File::decodeLine(buffer, params)) {
        errno = EUCLEAN;
        cerr << "Warning: in file " << config_path << ", line " << line
          << " missing single- or double-quote" << endl;
      }
      if (params.size() == 1) {
        errno = EUCLEAN;
        cerr << "Error: in file " << config_path << ", line " << line
          << " unexpected lonely keyword: " << params[0] << endl;
        failed = 2;
      } else if (params.size() > 1) {
        if (params[0] == "db") {
          if (params.size() > 2) {
            cerr << "Error: in file " << config_path << ", line " << line
              << " '" << params[0] << "' takes exactly one argument" << endl;
            failed = 2;
          }
          db_path = params[1];
        } else if (params[0] == "client") {
          if (params.size() > 2) {
            cerr << "Error: in file " << config_path << ", line " << line
              << " '" << params[0] << "' takes exactly one argument" << endl;
            failed = 2;
          }
          client = new Client(params[1]);
          clients.push_back(client);
        } else if (client != NULL) {
          if (params[0] == "hostname") {
            if (params.size() > 2) {
              cerr << "Error: in file " << config_path << ", line " << line
                << " '" << params[0] << "' takes exactly one argument" << endl;
              failed = 2;
            }
            client->setHostOrIp(params[1]);
          } else
          if (params[0] == "protocol") {
            if (params.size() > 2) {
              cerr << "Error: in file " << config_path << ", line " << line
                << " '" << params[0] << "' takes exactly one argument" << endl;
              failed = 2;
            }
            client->setProtocol(params[1]);
          } else
          if (params[0] == "option") {
            if (params.size() > 3) {
              cerr << "Error: in file " << config_path << ", line " << line
                << " '" << params[0] << "' takes one or two arguments" << endl;
              failed = 2;
            }
            if (params.size() == 2) {
              client->addOption(params[1]);
            } else {
              client->addOption(params[1], params[2]);
            }
          } else
          if (params[0] == "listfile") {
            if (params.size() > 2) {
              cerr << "Error: in file " << config_path << ", line " << line
                << " '" << params[0] << "' takes exactly one argument" << endl;
              failed = 2;
            }
            client->setListfile(params[1]);
          } else {
            cerr << "Unrecognised keyword '" << params[0]
              << "' in configuration file, line " << line
              << endl;
            failed = 2;
          }
        } else {
          cerr << "Error: in file " << config_path << ", line " << line
            << " unknown keyword" << endl;
          failed = 2;
        }
      }
    }
    config_file.close();

    if (! failed) {
      if (db_path == "") {
        db_path = default_db_path;
      }
      /* Open backup database */
      Database db(db_path);
      if (! db.open()) {
        if (check) {
          db.scan("", true);
        } else
        if (scan) {
          db.scan();
        } else {
          /* Backup */
          if (clients.backup(db, configcheck)) {
            failed = 1;
          }
        }
        db.close();
      } else {
        return 2;
      }
    }
  }
  return failed;
}
