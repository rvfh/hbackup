/* Herve Fache

20061006 Creation
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

/* Verbosity */
static int verbose = 0;

/* Signal received? */
static int killed = 0;

static void show_version(void) {
  printf("(c) 2006 Hervé Fache, version 0.1.\n");
}

static void show_help(void) {
  show_version();
  printf("Options are:\n");
  printf(" -h or --help to print this help and exit\n");
  printf(" -v or --version to print version and exit\n");
  printf(" -c or --config to specify a configuration file other than \
/etc/hbackup.conf\n");
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
  char config_path[FILENAME_MAX] = "/etc/hbackup.conf";
  char db_path[FILENAME_MAX]     = "/hbackup";
  char *mount_path;
  FILE *config;
  int failed = 0;
  int expect_configpath = 0;
  struct sigaction action;

  /* Set signal catcher */
  action.sa_handler = sighandler;
  sigemptyset (&action.sa_mask);
  action.sa_flags = 0;
  sigaction (SIGINT, &action, NULL);
  sigaction (SIGHUP, &action, NULL);
  sigaction (SIGTERM, &action, NULL);

  /* Analyse arguments */
  if (argc > 1) {
    char letter = ' ';

    /* -* */
    if (argv[1][0] == '-') {
      /* --* */
      if (argv[1][1] == '-') {
        if (! strcmp(&argv[1][2], "config")) {
          letter = 'c';
        } else if (! strcmp(&argv[1][2], "help")) {
          letter = 'h';
        } else if (! strcmp(&argv[1][2], "verbose")) {
          letter = 'v';
        } else if (! strcmp(&argv[1][2], "version")) {
          letter = 'V';
        }
      } else if (argv[1][1] == 'v') {
        letter = argv[1][1];
        if (argv[1][2] == 'v') {
          verbose++;
          if (argv[1][3] == 'v') {
            verbose++;
            if (argv[1][4] != '\0') {
              letter = ' ';
            }
          } else if (argv[1][3] != '\0') {
            letter = ' ';
          }
        } else if (argv[1][2] != '\0') {
          letter = ' ';
        }
      } else if (argv[1][2] == '\0') {
        letter = argv[1][1];
      }

      switch (letter) {
        case 'c':
          expect_configpath = 1;
          break;
        case 'h':
          show_help();
          return 0;
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

    /* Get config path */
    if (expect_configpath) {
      if (argc < 3) {
        fprintf(stderr, "Need a config path after %s\n", argv[1]);
        return 2;
      }
      strcpy(config_path, argv[2]);
    }
  }

  if (verbosity() > 2) {
    printf("Verbosity level: %u\n", verbosity());
  }

  /* Open configuration file */
  if ((config = fopen(config_path, "r")) == NULL) {
    fprintf(stderr, "Configuration file not found %s\n", config_path);
    return 2;
  } else {
    /* Read configuration file */
    char *buffer = malloc(FILENAME_MAX);
    size_t size;
    int line = 0;

    if (clients_new()) {
      fprintf(stderr, "Failed to create clients list\n");
    } else {
      while (getline(&buffer, &size, config) >= 0) {
        char keyword[256];
        char type[256];
        char string[FILENAME_MAX];
        int params = params_readline(buffer, keyword, type, string);

        line++;
        if (params > 1) {
          if (! strcmp(keyword, "db")) {
            strcpy(db_path, string);
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
      }
      fclose(config);
      free(buffer);

      if (! failed) {
        /* Open backup database */
        if (db_open(db_path) == 2) {
          fprintf(stderr, "Failed to open database in '%s'\n", db_path);
          failed = 2;
        } else {
          /* Make sure we have a mount path */
          one_trailing_slash(db_path);
          mount_path = malloc(strlen(db_path) + strlen("mount/") + 1);
          sprintf(mount_path, "%s%s", db_path, "mount/");
          if (testdir(mount_path, 1) == 2) {
            fprintf(stderr, "Failed to create mount point\n");
            failed = 2;
          } else

          /* Backup */
          if (clients_backup(mount_path)) {
            fprintf(stderr, "Failed to backup\n");
            failed = 1;
          }

          db_close();
        }
      }
      clients_free();
    }
  }

  return failed;
}
