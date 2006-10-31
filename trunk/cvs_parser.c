/* Herve Fache

20061005 Creation
*/

#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include "list.h"
#include "metadata.h"
#include "parsers.h"
#include "cvs_parser.h"

/* Our data */
typedef struct {
  char    *name;  /* File name */
  mode_t  type;   /* File type */
} cvs_entry_t;

static char *cvs_payload_get(const void *payload) {
  char *string = NULL;

  asprintf(&string, ((cvs_entry_t *) payload)->name);
  return string;
}

static int cvs_dir_check(void **handle, const char *dir_path) {
  char    *d_path = NULL;
  FILE    *entries;
  int     failed = 0;

  asprintf(&d_path, "%s/CVS/Entries", dir_path);

  /* Check that file exists */
  if ((entries = fopen(d_path, "r")) == NULL) {
    /* Directory is not under CVS control */
    *handle = NULL;
    failed = 1;
  } else {
    list_t  list = list_new(cvs_payload_get);
    char    *buffer = NULL;
    size_t  size    = 0;

    /* Return list */
    *handle = list;

    /* Fill in list of controlled files */
    while (getline(&buffer, &size, entries) >= 0) {
      cvs_entry_t cvs_entry;
      cvs_entry_t *payload;
      char *start = buffer;
      char *delim;

      if (start[0] == 'D') {
        cvs_entry.type = S_IFDIR;
        start++;
      } else {
        cvs_entry.type = S_IFREG;
      }
      if (start[0] != '/') {
        continue;
      }
      start++;
      if ((delim = strchr(start, '/')) == NULL) {
        continue;
      }

      *delim = '\0';
      asprintf(&cvs_entry.name, start);

      payload = malloc(sizeof(cvs_entry_t));
      *payload = cvs_entry;
      list_add(list, payload);
    }
    /* Close file */
    fclose(entries);
    free(buffer);
  }
  free(d_path);
  return failed;
}

static void cvs_dir_leave(void *list) {
  list_entry_t entry = NULL;

  while ((entry = list_next(list, entry)) != NULL) {
    cvs_entry_t *cvs_entry = list_entry_payload(entry);

    free(cvs_entry->name);
  }
  list_free(list);
}


/* Ideally, this function should return 0 when the file is added or modified */
/* For now, it returns 0 for any file under CVS control */
static int cvs_file_check(void *list, const filedata_t *file_data) {
  if (list == NULL) {
    fprintf(stderr, "cvs parser: file check: not initialised\n");
    return 2;
  } else {
    const char *file = strrchr(file_data->path, '/');

    if (file != NULL) {
      file++;
    } else {
      file = file_data->path;
    }
    return list_find(list, file, NULL, NULL);
  }
}

/* That's the parser */
static parser_t cvs_parser = { cvs_dir_check, cvs_dir_leave, cvs_file_check, NULL, "cvs" };

parser_t *cvs_parser_new(void) {
  /* This needs to be dynamic memory */
  parser_t *parser = malloc(sizeof(parser_t));

  *parser = cvs_parser;
  return parser;
}
