/* Herve Fache

20061008 Creation
*/

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <regex.h>
#include "metadata.h"
#include "list.h"
#include "filters.h"

typedef struct {
  filter_type_t type;
  mode_t        file_type;
  size_t        size;
  char          string[256];
} filter_t;

static int filter_path_end_check(const char *path, const filter_t *filter) {
  signed int diff = strlen(path) - strlen(filter->string);

  if (diff < 0) {
    return 1;
  }
  return strcmp(filter->string, &path[diff]);
}

static int filter_path_regexp_check(const char *path, const filter_t *filter) {
  regex_t regex;

  if (regcomp(&regex, filter->string, REG_EXTENDED)) {
    fprintf(stderr, "filters: regexp: incorrect expression\n");
    return 2;
  }
  return regexec(&regex, path, 0, NULL, 0);
}

static int filter_path_start_check(const char *path, const filter_t *filter) {
  return strncmp(path, filter->string, strlen(filter->string));
}

int filters_new(void **handle) {
  /* Create new list */
  *handle = list_new(NULL);
  if (*handle == NULL) {
    fprintf(stderr, "filters: new: cannot intialise\n");
    return 2;
  }
  return 0;
}

void filters_free(void *handle) {
  list_free(handle);
}

int filters_add(void *handle, mode_t file_type, filter_type_t type, ...) {
  filter_t *filter = malloc(sizeof(filter_t));
  va_list  args;

  if (filter == NULL) {
    fprintf(stderr, "filters: add: cannot allocate memory\n");
    return 2;
  }
  filter->type = type;

  va_start(args, type);
  switch (type) {
    case filter_path_end:
    case filter_path_start:
    case filter_path_regexp:
      strcpy(filter->string, va_arg(args, char *));
      filter->file_type = file_type;
      break;
    case filter_size_above:
    case filter_size_below:
      filter->size      = va_arg(args, size_t);
      filter->file_type = S_IFREG;
      break;
    default:
      fprintf(stderr, "filters: add: unknown filter type\n");
      return 2;
  }
  va_end(args);

  if (list_add(handle, filter) == NULL) {
    fprintf(stderr, "filters: add: failed\n");
    return 2;
  }
  return 0;
}

int filters_match(void *handle, const filedata_t *filedata) {
  list_entry_t *entry = NULL;

  while ((entry = list_next(handle, entry)) != NULL) {
    filter_t *filter = list_entry_payload(entry);

    if ((filter->file_type & filedata->metadata.type) == 0) {
      continue;
    }
    switch(filter->type) {
    case filter_path_end:
      if (! filter_path_end_check(filedata->path, filter)) {
        return 0;
      }
      break;
    case filter_path_start:
      if (! filter_path_start_check(filedata->path, filter)) {
        return 0;
      }
      break;
    case filter_path_regexp:
      if (! filter_path_regexp_check(filedata->path, filter)) {
        return 0;
      }
      break;
    case filter_size_above:
      if (filedata->metadata.size > filter->size) {
        return 0;
      }
      break;
    case filter_size_below:
      if (filedata->metadata.size < filter->size) {
        return 0;
      }
      break;
    default:
      fprintf(stderr, "filters: match: unknown filter type\n");
      return 2;
    }
  }
  /* No match */
  return 1;
}
