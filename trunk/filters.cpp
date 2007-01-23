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
  off_t         size;
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

int filters_new(list_t **handle) {
  /* Create new list */
  *handle = list_new(NULL);
  if (*handle == NULL) {
    fprintf(stderr, "filters: new: failed\n");
    return 2;
  }
  return 0;
}

void filters_free(list_t *handle) {
  /* Note: entry gets free in the loop */
  list_entry_t *entry;

  /* List of lists, free each embedded list */
  while ((entry = list_next(handle, NULL)) != NULL) {
    list_free((list_t *) (list_remove(handle, entry)));
  }
  if (list_size(handle) != 0) {
    fprintf(stderr, "filters: free: failed\n");
  }
  list_free(handle);
}

list_t *filters_rule_new(list_t *handle) {
  list_t *rule = list_new(NULL);

  if (list_append(handle, rule) == NULL) {
    fprintf(stderr, "filters: rule new: failed\n");
    return NULL;
  }
  return rule;
}

int filters_rule_add(list_t *rule_handle, mode_t file_type,
    filter_type_t type, ...) {
  filter_t *filter = new filter_t;
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

  if (list_append(rule_handle, filter) == NULL) {
    fprintf(stderr, "filters: add: failed\n");
    return 2;
  }
  return 0;
}

static int filter_match(const filter_t *filter, const filedata_t *filedata) {
  /* Check that file type matches */
  if ((filter->file_type & filedata->metadata.type) == 0) {
    return 1;
  }
  /* Run filters */
  switch(filter->type) {
  case filter_path_end:
    return filter_path_end_check(filedata->path, filter);
  case filter_path_start:
    return filter_path_start_check(filedata->path, filter);
  case filter_path_regexp:
    return filter_path_regexp_check(filedata->path, filter);
  case filter_size_above:
    return filedata->metadata.size < filter->size;
  case filter_size_below:
    return filedata->metadata.size > filter->size;
  default:
    fprintf(stderr, "filters: match: unknown filter type\n");
    return 2;
  }
  return 1;
}

int filters_match(const list_t *handle, const filedata_t *filedata) {
  list_entry_t *rule_entry = NULL;

  /* Read through list of rules */
  while ((rule_entry = list_next(handle, rule_entry)) != NULL) {
    list_t       *rule          = (list_t *) (list_entry_payload(rule_entry));
    list_entry_t *filter_entry  = NULL;
    int          match          = 1;

    /* Read through list of filters in rule */
    while ((filter_entry = list_next(rule, filter_entry)) != NULL) {
      filter_t *filter = (filter_t *) (list_entry_payload(filter_entry));

      /* All filters must match for rule to match */
      if (filter_match(filter, filedata)) {
        match = 0;
        break;
      }
    }
    /* If all filters matched (or the rule is empty!), we have a rule match */
    if (match) {
      return 0;
    }
  }
  /* No match */
  return 1;
}
