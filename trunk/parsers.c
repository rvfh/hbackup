/* Herve Fache

20061005 Creation
*/

#include <stdlib.h>
#include "list.h"
#include "parsers.h"

int parsers_new(void **handle) {
  /* Create new list */
  *handle = list_new(NULL);
  if (*handle == NULL) {
    fprintf(stderr, "parsers: new: cannot intialise\n");
    return 2;
  }
  return 0;
}

void parsers_free(void *handle) {
  list_free(handle);
}

int parsers_add(void *handle, parser_t *parser) {
  if (list_append(handle, parser) == NULL) {
    fprintf(stderr, "parsers: add: failed\n");
    return 2;
  }
  return 0;
}

int parsers_dir_check(const void *parsers_handle, parser_t **parser_handle,
    void **dir_handle, const char *path) {
  if (*parser_handle == NULL) {
    const list_t parsers_list = (const list_t) parsers_handle;
    list_entry_t *entry       = NULL;

    while ((entry = list_next(parsers_list, entry)) != NULL) {
      *parser_handle = list_entry_payload(entry);
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
