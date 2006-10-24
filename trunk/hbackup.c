/* Herve Fache

20061006 Creation
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "params.h"
#include "list.h"
#include "filters.h"
#include "parsers.h"
#include "filelist.h"
#include "cvs_parser.h"
#include "db.h"
#include "clients.h"

/* List of files */
list_t file_list;

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

int main(int argc, char **argv) {
  char config_path[FILENAME_MAX] = "/etc/hbackup.conf";
  char db_path[FILENAME_MAX] = "/hbackup";
  FILE *config;
  int failed = 0;

  /* Analyse arguments */
  if (argc > 1) {
    char letter = ' ';

    /* -* */
    if (argv[1][0] == '-') {
      /* --* */
      if (argv[1][1] == '-') {
        if (! strcmp(&argv[1][2], "help")) {
          letter = 'h';
        } else if (! strcmp(&argv[1][2], "version")) {
          letter = 'v';
        } else if (! strcmp(&argv[1][2], "config")) {
          letter = 'c';
        }
      } else if (argv[1][2] == '\0') {
        letter = argv[1][1];
      }
      switch (letter) {
        case 'h':
          show_help();
          return 0;
        case 'v':
          show_version();
          return 0;
        case 'c':
          break;
        default:
          fprintf(stderr, "Unrecognised option: %s\n", argv[1]);
          return 2;
      }
    }

    /* Get config path */
    if (argc < 3) {
      fprintf(stderr, "Need a config path after %s\n", argv[1]);
      return 2;
    }
    strcpy(config_path, argv[2]);
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

      /* Open backup database */
      if (db_open(db_path) == 2) {
        fprintf(stderr, "Failed to open database in '%s'\n", db_path);
        failed = 2;
      }

      /* Backup */
      if (! failed && clients_backup()) {
        fprintf(stderr, "Failed to backup\n");
      }
      db_close();
      clients_free();
    }
  }

  return failed;
}
