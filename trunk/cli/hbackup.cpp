/*
     Copyright (C) 2006-2007  Herve Fache

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

#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <signal.h>
#include <errno.h>

using namespace std;

#include "hbackup.h"
#include "db.h"
#include "clients.h"

using namespace hbackup;

/* DEFAULTS */

/* Verbosity */
static int verbose = 0;

/* Configuration path */
static string default_config_path = "/etc/hbackup/hbackup.conf";

/* Configuration path */
static string default_db_path = "/hbackup";


/* Signal received? */
static int killed = 0;

static void show_version(void) {
  cout << "(c) 2006-2007 Hervé Fache, version "
    << VERSION_MAJOR << "." << VERSION_MINOR;
  if (VERSION_BUGFIX != 0) {
    cout << "." << VERSION_BUGFIX;
  }
  if (BUILD != 0) {
    cout << " (build " << BUILD << ")";
  }
  cout << endl;
}

static void show_help(void) {
  show_version();
  cout << "Options are:" << endl;
  cout << " -h or --help     to print this help and exit" << endl;
  cout << " -V or --version  to print version and exit" << endl;
  cout << " -c or --config   to specify a configuration file other than \
/etc/hbackup/hbackup.conf" << endl;
  cout << " -s or --scan     to scan the database for missing data" << endl;
  cout << " -t or --check    to check the database for corrupted data" << endl;
  cout << " -u or --update   to only update the database format" << endl;
  cout << " -v or --verbose  to be more verbose (also -vv and -vvv)" << endl;
  cout << " -C or --client   specify client to backup (more than one allowed)"
    << endl;
}

int verbosity(void) {
  return verbose;
}

int terminating(void) {
  return killed;
}

void sighandler(int signal) {
  if (killed) {
    cerr << "Already aborting..." << endl;
  } else {
    cerr << "Signal received, aborting..." << endl;
  }
  killed = signal;
}

int main(int argc, char **argv) {
  list<string>      requested_clients;
  string            config_path       = "";
  string            db_path           = "";
  int               failed            = 0;
  int               argn              = 0;
  bool              scan              = false;
  bool              check             = false;
  bool              config_check      = false;
  bool              update            = false;
  bool              expect_configpath = false;
  bool              expect_client     = false;
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

    /* Get config path if request */
    if (expect_client) {
      requested_clients.push_back(argv[argn]);
      expect_client = false;
    }

    /* -* */
    if (argv[argn][0] == '-') {
      /* --* */
      if (argv[argn][1] == '-') {
        if (! strcmp(&argv[argn][2], "config")) {
          letter = 'c';
        } else if (! strcmp(&argv[argn][2], "debug")) {
          letter = 'd';
        } else if (! strcmp(&argv[argn][2], "help")) {
          letter = 'h';
        } else if (! strcmp(&argv[argn][2], "restore")) {
          letter = 'r';
        } else if (! strcmp(&argv[argn][2], "scan")) {
          letter = 's';
        } else if (! strcmp(&argv[argn][2], "check")) {
          letter = 't';
        } else if (! strcmp(&argv[argn][2], "configcheck")) {
          letter = 'p';
        } else if (! strcmp(&argv[argn][2], "update")) {
          letter = 'u';
        } else if (! strcmp(&argv[argn][2], "verbose")) {
          letter = 'v';
        } else if (! strcmp(&argv[argn][2], "client")) {
          letter = 'C';
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
        case 'd':
          verbose = 9;
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
        case 'p':
          config_check = true;
          break;
        case 'u':
          update = true;
          break;
        case 'v':
          verbose++;
          break;
        case 'C':
          expect_client = true;
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
    cerr << "Missing config path" << endl;
    return 2;
  }

  if (config_path == "") {
    config_path = default_config_path;
  }

  /* Open configuration file */
  ifstream config_file(config_path.c_str());

  if (! config_file.is_open()) {
    cerr << strerror(errno) << " " << config_path << endl;
    failed = 2;
  } else {
    /* Read configuration file */
    list<Client*> clients;
    string  buffer;
    int     line    = 0;

    if (verbosity() > 1) {
      cout << " -> Reading configuration file" << endl;
    }

    Client* client = NULL;
    while (! config_file.eof() && ! failed) {
      getline(config_file, buffer);
      unsigned int pos = buffer.find("\r");
      if (pos != string::npos) {
        buffer.erase(pos);
      }
      list<string> params;

      line++;
      if (Stream::decodeLine(buffer, params)) {
        errno = EUCLEAN;
        cerr << "Warning: in file " << config_path << ", line " << line
          << " missing single- or double-quote" << endl;
      }
      if (params.size() == 1) {
        errno = EUCLEAN;
        cerr << "Error: in file " << config_path << ", line " << line
          << " unexpected lonely keyword: " << *params.begin() << endl;
        failed = 2;
      } else if (params.size() > 1) {
        list<string>::iterator current = params.begin();
        string                 keyword = *current++;

        if (keyword == "db") {
          if (params.size() > 2) {
            cerr << "Error: in file " << config_path << ", line " << line
              << " '" << keyword << "' takes exactly one argument" << endl;
            failed = 2;
          }
          db_path = *current;
        } else if (keyword == "client") {
          if (params.size() > 2) {
            cerr << "Error: in file " << config_path << ", line " << line
              << " '" << keyword << "' takes exactly one argument" << endl;
            failed = 2;
          }
          client = new Client(*current);
          clients.push_back(client);
        } else if (client != NULL) {
          if (keyword == "hostname") {
            if (params.size() > 2) {
              cerr << "Error: in file " << config_path << ", line " << line
                << " '" << keyword << "' takes exactly one argument" << endl;
              failed = 2;
            }
            client->setHostOrIp(*current);
          } else
          if (keyword == "protocol") {
            if (params.size() > 2) {
              cerr << "Error: in file " << config_path << ", line " << line
                << " '" << keyword << "' takes exactly one argument" << endl;
              failed = 2;
            }
            client->setProtocol(*current);
          } else
          if (keyword == "option") {
            if (params.size() > 3) {
              cerr << "Error: in file " << config_path << ", line " << line
                << " '" << keyword << "' takes one or two arguments" << endl;
              failed = 2;
            }
            if (params.size() == 2) {
              client->addOption(*current);
            } else {
              string name = *current++;
              client->addOption(name, *current);
            }
          } else
          if (keyword == "listfile") {
            if (params.size() > 2) {
              cerr << "Error: in file " << config_path << ", line " << line
                << " '" << keyword << "' takes exactly one argument" << endl;
              failed = 2;
            }
            client->setListfile(*current);
          } else {
            cerr << "Unrecognised keyword '" << keyword
              << "' in configuration file, line " << line
              << endl;
            failed = 2;
          }
        } else {
          cerr << "Error: in file " << config_path << ", line " << line
            << " keyword before client" << endl;
          failed = 2;
        }
      }
    }
    config_file.close();

    if (! failed) {
      if (db_path == "") {
        db_path = default_db_path;
      }
      // Open backup database
      Database db(db_path);
      if (verbosity() > 1) {
        cout << " -> Opening database" << endl;
      }
      if (! db.open()) {
        if (update) {
          if (verbosity() > 1) {
            cout << " -> Updating database format" << endl;
          }
        } else
        if (check) {
          if (verbosity() > 1) {
            cout << " -> Checking database" << endl;
          }
          db.scan("", true);
        } else
        if (scan) {
          if (verbosity() > 1) {
            cout << " -> Scanning database" << endl;
          }
          db.scan();
        } else {
          /* Backup */
          for (list<Client*>::iterator client = clients.begin();
              client != clients.end(); client++) {
            if (terminating()) {
              break;
            }
            // Skip unrequested clients
            if (requested_clients.size() != 0) {
              bool found = false;
              for (list<string>::iterator i = requested_clients.begin();
               i != requested_clients.end(); i++) {
                if (*i == (*client)->name()) {
                  found = true;
                  break;
                }
              }
              if (! found) {
                continue;
              }
            }
            (*client)->setMountPoint(db_path + "/mount");
            if ((*client)->backup(db, config_check)) {
              failed = 1;
            }
          }
        }
        if (verbosity() > 1) {
          cout << " -> Closing database" << endl;
        }
        db.close();
      } else {
        return 2;
      }
    }
    // Delete clients
    for (list<Client*>::iterator client = clients.begin();
        client != clients.end(); client++){
      delete *client;
    }
  }
  return failed;
}
