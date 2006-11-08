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

parser_t *parsers_next(void *handle, void **parser_handle) {
  list_entry_t *entry = *parser_handle;

  *parser_handle = entry = list_next(handle, entry);
  if (entry != NULL) {
    return (parser_t *) list_entry_payload(entry);
  } else {
    return NULL;
  }
}
