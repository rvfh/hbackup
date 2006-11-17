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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "params.h"
#include "list.h"
#include "tools.h"
#include "filters.h"
#include "parsers.h"
#include "filelist.h"
#include "cvs_parser.h"
#include "db.h"
#include "clients.h"
#include "hbackup.h"
#include "version.h"

/* DEFAULTS */

/* Verbosity */
static int verbose = 0;

/* Configuration path */
static const char default_config_path[] = "/etc/hbackup.conf";

/* Configuration path */
static const char default_db_path[] = "/hbackup";


/* Signal received? */
static int killed = 0;

static void show_version(void) {
  printf("(c) 2006 Hervé Fache, version %u.%u", VERSION_MAJOR, VERSION_MINOR);
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
  const char       *config_path      = NULL;
  char             *db_path          = NULL;
  FILE             *config;
  int              failed            = 0;
  int              expect_configpath = 0;
  int              argn              = 0;
  int              scan              = 0;
  int              check             = 0;
  int              configcheck       = 0;
  struct sigaction action;

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
      config_path = argv[argn];
      expect_configpath = 0;
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
          expect_configpath = 1;
          break;
        case 'h':
          show_help();
          return 0;
        case 's':
          scan = 1;
          break;
        case 't':
          check = 1;
          break;
        case 'u':
          configcheck = 1;
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

  if (config_path == NULL) {
    config_path = default_config_path;
  }

  if (verbosity() > 2) {
    printf("Verbosity level: %u\n", verbosity());
  }

  /* Open configuration file */
  if ((config = fopen(config_path, "r")) == NULL) {
    fprintf(stderr, "Configuration file not found %s\n", config_path);
    failed = 2;
  } else {
    /* Read configuration file */
    char *buffer = NULL;
    size_t size  = 0;
    int line = 0;

    if (clients_new()) {
      fprintf(stderr, "Failed to create clients list\n");
    } else {
      while (getline(&buffer, &size, config) >= 0) {
        char keyword[256];
        char type[256];
        char *string = malloc(size);
        int params = params_readline(buffer, keyword, type, string);

        line++;
        if (params > 1) {
          if (! strcmp(keyword, "db")) {
            asprintf(&db_path, "%s", string);
          } else if (! strcmp(keyword, "client")) {
            if (clients_add(type, string)) {
              fprintf(stderr,
                "Syntax error in configuration file %s, line %u\n",
                config_path, line);
              failed = 2;
            }
          } else {
            fprintf(stderr,
              "Unrecognised keyword in configuration file %s, line %u\n",
              config_path, line);
            failed = 2;
          }
        }
        free(string);
      }
      fclose(config);
      free(buffer);

      if (! failed && ! configcheck) {
        if (db_path == NULL) {
          asprintf(&db_path, "%s", default_db_path);
        }
        /* Open backup database */
        if (check) {
          db_check(db_path, NULL);
        } else
        if (scan) {
          db_scan(db_path, NULL);
        } else
        if (db_open(db_path) == 2) {
          fprintf(stderr, "Failed to open database in '%s'\n", db_path);
          failed = 2;
        } else {
          char *mount_path = NULL;

          /* Make sure we have a mount path */
          asprintf(&mount_path, "%s/mount", db_path);
          if (testdir(mount_path, 1) == 2) {
            fprintf(stderr, "Failed to create mount point\n");
            failed = 2;
          } else

          /* Backup */
          if (clients_backup(mount_path, configcheck)) {
            fprintf(stderr, "Failed to backup\n");
            failed = 1;
          }
          free(mount_path);
          db_close();
        }
      }
      clients_free();
    }
  }
  free(db_path);

  return failed;
}
