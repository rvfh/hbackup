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
  char    name[FILENAME_MAX]; /* Name */
  mode_t  type;               /* Directory? */
/*  char    mtime[32];*/          /* File modified time */
} cvs_entry_t;

static void cvs_payload_get(const void *payload, char *string) {
  strcpy(string, ((cvs_entry_t *) payload)->name);
}

static int cvs_dir_check(void **handle, const char *dir_path) {
  list_t list;
  char d_path[FILENAME_MAX];
  char *buffer = malloc(256);
  size_t size;
  FILE *entries;

  strcpy(d_path, dir_path);
  strcat(d_path, "/CVS/Entries");
  /* Check that file exists */
  if ((entries = fopen(d_path, "r")) == NULL) {
    /* Directory is not under CVS control */
    *handle = NULL;
    return 1;
  }
  /* Create new list */
  *handle = list = list_new(cvs_payload_get);

  /* Fill in list of controlled files */
  while (getline(&buffer, &size, entries) >= 0) {
    cvs_entry_t cvs_entry;
    cvs_entry_t *payload;
    char *line = buffer;
    char *delim;

    if (line[0] == 'D') {
      cvs_entry.type = S_IFDIR;
      line++;
    } else {
      cvs_entry.type = S_IFREG;
    }
    if (line[0] != '/') {
      continue;
    }
    line++;
    strncpy(cvs_entry.name, line, FILENAME_MAX);
    if ((delim = strchr(cvs_entry.name, '/')) == NULL) {
      continue;
    }
    *delim = '\0';
/*    if (S_ISREG(cvs_entry.type)) {
      line = strchr(line, '/');
      line++;
      line = strchr(line, '/');
      line++;
      if ((delim = strchr(line, '/')) == NULL) {
        continue;
      }
      *delim = '\0';
      strncpy(cvs_entry.mtime, line, 32);
      fprintf(stderr, "%u\n", strlen(cvs_entry.mtime));
      cvs_entry.mtime[1] = '\0';
    } else {
      cvs_entry.mtime[0] = '\0';
    }*/
    payload = malloc(sizeof(cvs_entry_t));
    *payload = cvs_entry;
    list_add(list, payload);
  }
  /* Close file */
  fclose(entries);
  free(buffer);
  return 0;
}

static void cvs_dir_leave(void *list) {
  list_free(list);
}


/* Ideally, this function should return 0 when the file is added or modified */
/* For now, it returns 0 for any file under CVS control */
static int cvs_file_check(void *list, const filedata_t *file_data) {
  if (list == NULL) {
    fprintf(stderr, "cvs parser: file check: not initialised\n");
    return 2;
  } else {
    char path[FILENAME_MAX];
    char *file;

    strcpy(path, file_data->path);
    if ((file = strrchr(path, '/')) != NULL) {
      file++;
    } else {
      file = path;
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
