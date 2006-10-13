/* Herve Fache

20061005 Creation
*/

#include <stdlib.h>
#include "list.h"
#include "parsers.h"

static list_t parsers_list = NULL;

int parsers_new(void) {
  if ((parsers_list = list_new(NULL)) == NULL) {
    fprintf(stderr, "parsers: new: cannot initialise\n");
    return 2;
  }
  return 0;
}

void parsers_free(void) {
  /* First destroy all parsers */
  list_entry_t *entry = NULL;
  while ((entry = list_next(parsers_list, entry)) != NULL) {
    parser_t *parser = (parser_t *) list_entry_payload(entry);

    if (parser->destroy != NULL) {
      parser->destroy();
    }
  }
  /* Then free list */
  list_free(parsers_list);
}

int parsers_add(parser_t *parser) {
  if (list_add(parsers_list, parser)) {
    fprintf(stderr, "parsers: add: failed\n");
    return 2;
  }
  return 0;
}

parser_t *parsers_next(void **handle) {
  list_entry_t *entry = *handle;

  *handle = entry = list_next(parsers_list, entry);
  if (entry != NULL) {
    return (parser_t *) list_entry_payload(entry);
  } else {
    return NULL;
  }
}
