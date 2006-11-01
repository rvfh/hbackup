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
  char          string[256];
  size_t        size;
  filter_type_t type;
} filter_t;

static int filter_end_check(const char *path, const char *string) {
  signed int diff = strlen(path) - strlen(string);

  if (diff < 0) {
    return 1;
  }
  return strcmp(string, &path[diff]);
}

static int filter_path_start_check(const char *path, const char *string) {
  return strncmp(path, string, strlen(string));
}

static int filter_path_regexp_check(const char *path, const char *string) {
  regex_t regex;

  if (regcomp(&regex, string, REG_EXTENDED)) {
    fprintf(stderr, "filters: regexp: incorrect expression\n");
    return 2;
  }
  return regexec(&regex, path, 0, NULL, 0);
}

static int filter_file_start_check(const char *path, const char *string) {
  const char *file = strrchr(path, '/');

  if (file != NULL) {
    file++;
  } else {
    file = path;
  }
  return strncmp(file, string, strlen(string));
}

static int filter_file_regexp_check(const char *path, const char *string) {
  const char *file;

  if ((file = strrchr(path, '/')) != NULL) {
    file++;
  } else {
    file = path;
  }
  return filter_path_regexp_check(file, string);
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

int filters_add(void *handle, filter_type_t type, ...) {
  filter_t *filter = malloc(sizeof(filter_t));
  va_list  args;

  if (filter == NULL) {
    fprintf(stderr, "filters: add: cannot allocate memory\n");
    return 2;
  }
  filter->type = type;

  va_start(args, type);
  switch (type) {
    case filter_end:
    case filter_path_start:
    case filter_path_regexp:
    case filter_file_start:
    case filter_file_regexp:
      strcpy(filter->string, va_arg(args, char *));
      break;
    case filter_size_min:
    case filter_size_max:
      filter->size = va_arg(args, size_t);
      break;
    default:
      fprintf(stderr, "filters: add: unknown filter type\n");
      return 2;
  }
  va_end(args);

  if (list_add(handle, filter)) {
    fprintf(stderr, "filters: add: failed\n");
    return 2;
  }
  return 0;
}

int filters_match(void *handle, const filedata_t *filedata) {
  list_entry_t *entry = NULL;

  while ((entry = list_next(handle, entry)) != NULL) {
    filter_t *filter = list_entry_payload(entry);

    switch(filter->type) {
    case filter_end:
      if (! filter_end_check(filedata->path, filter->string)) {
        return 0;
      }
      break;
    case filter_path_start:
      if (! filter_path_start_check(filedata->path, filter->string)) {
        return 0;
      }
      break;
    case filter_path_regexp:
      if (! filter_path_regexp_check(filedata->path, filter->string)) {
        return 0;
      }
      break;
    case filter_file_start:
      if (! filter_file_start_check(filedata->path, filter->string)) {
        return 0;
      }
      break;
    case filter_file_regexp:
      if (! filter_file_regexp_check(filedata->path, filter->string)) {
        return 0;
      }
      break;
    case filter_size_min:
      if (S_ISREG(filedata->metadata.type) && (filedata->metadata.size > filter->size)) {
        return 0;
      }
      break;
    case filter_size_max:
      if (S_ISREG(filedata->metadata.type) && (filedata->metadata.size < filter->size)) {
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
