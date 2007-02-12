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

using namespace std;

#include <string>
#include "list.h"
#include "metadata.h"
#include "common.h"
#include "parsers.h"

int parsers_new(List **list) {
  /* Create new list */
  *list = new List();
  if (*list == NULL) {
    fprintf(stderr, "parsers: new: cannot intialise\n");
    return 2;
  }
  return 0;
}

void parsers_free(List *list) {
  delete list;
}

int parsers_add(List *list, parser_mode_t mode, parser_t *parser) {
  parser->mode = mode;
  if (list->append(parser) == NULL) {
    fprintf(stderr, "parsers: add: failed\n");
    return 2;
  }
  return 0;
}

int parsers_dir_check(const void *parsers_handle, parser_t **parser_handle,
    void **dir_handle, const char *path) {
  if (*parser_handle == NULL) {
    const List *parsers_list = (const List *) parsers_handle;
    list_entry_t *entry       = NULL;

    while ((entry = parsers_list->next(entry)) != NULL) {
      *parser_handle = (parser_t *) (list_entry_payload(entry));
      if ((*parser_handle)->dir_check(dir_handle, path)
          == parser_dir_controlled) {
        return 0;
      }
    }
    *dir_handle    = NULL;
    *parser_handle = NULL;
    return 1;
  } else if ((*parser_handle)->dir_check(dir_handle, path)
      == parser_dir_controlled) {
    return 0;
  }
  return 2;
}

int parsers_file_check(parser_t *parser_handle, void *dir_handle,
    const filedata_t *filedata) {
  parser_file_status_t result;

  if ((parser_handle == NULL) || (dir_handle == NULL)) {
    /* If no parser, match everything */
    return 0;
  }
  result = parser_handle->file_check(dir_handle, filedata);
  /* Special case: we can't decide, so match! */
  if (result == parser_file_unknown) {
    return 0;
  }
  switch (parser_handle->mode) {
    case parser_disabled:
      /* Always matches */
      return 0;
    case parser_controlled:
      /* Matches any file under control */
      if (  (result == parser_file_maybemodified)
         || (result == parser_file_controlled)
         || (result == parser_file_modified)) {
        return 0;
      }
      break;
    case parser_modified:
      /* Matches any modified file under control */
      if (  (result == parser_file_maybemodified)
         || (result == parser_file_modified)) {
        return 0;
      }
      break;
    case parser_modifiedandothers:
      /* Matches any file unless it is under control and not modified */
      if (  (result == parser_file_maybemodified)
         || (result == parser_file_modified)
         || (result == parser_file_other)) {
        return 0;
      }
      break;
    case parser_others:
      /* Matches any file not under control */
      if (result == parser_file_other) {
        return 0;
      }
      break;
    default:
      return 2;
  }
  return 1;
}
