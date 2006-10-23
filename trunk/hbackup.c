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

static void file_data_show(const void *payload, char *string) {
  const filedata_t *filedata = payload;

  sprintf(string, "%s", filedata->path);
}

int main(int argc, char **argv) {
  char config_path[FILENAME_MAX] = "/etc/hbackup.conf";
  char db_path[FILENAME_MAX] = "/h  backup";
  FILE *config;
  void *filters_handle = NULL;
  void *parsers_handle = NULL;

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
    int failed = 0;

    clients_new();
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
            fprintf(stderr, "Syntax error in configuration file %s, line %u\n",
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
    if (failed) {
      return failed;
    }
  }

  parsers_new(&parsers_handle);
  parsers_add(parsers_handle, cvs_parser_new());

  filters_new(&filters_handle);
  filters_add(filters_handle, "test/subdir", filter_path_start);
  filters_add(filters_handle, "~", filter_end);
  filters_add(filters_handle, "\\.o$", filter_file_regexp);
  filters_add(filters_handle, ".svn", filter_file_start);

  filelist_new("test////", filters_handle, parsers_handle);
  list_show(filelist_getlist(), NULL, file_data_show);

  db_open("test_db");
  db_parse("file://host/share", filelist_getpath(), filelist_getlist());
  db_close();

  filelist_free();
  filters_free(filters_handle);
  parsers_free(parsers_handle);
  return 0;
}
