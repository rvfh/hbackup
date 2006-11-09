/* Herve Fache

20061005 Creation
*/

#define _GNU_SOURCE
#include <stdio.h>
#include "parsers.c"

static parser_t parser = {
  (parsers_dir_check_f *) 0x12345678,
  (parsers_dir_leave_f *) 0xDEADBEEF,
  (parsers_file_check_f *) 0x34567890,
  parser_disabled,
  "test parser" };

/* Use payload as argument name, cast once and for all */
static char *parsers_show(const void *payload) {
  const parser_t *parser = payload;
  char *string = NULL;

  asprintf(&string, "%s [0x%08x, 0x%08x, 0x%08x]",
    parser->name,
    (unsigned int) parser->dir_check,
    (unsigned int) parser->dir_leave,
    (unsigned int) parser->file_check);
  return string;
}

/* TODO parsers_dir_check test */
int main(void) {
  parser_t *parser_p = malloc(sizeof(parser_t));
  void *handle1 = NULL;
  void *handle2 = NULL;

  if (parsers_new(&handle1)) {
    printf("Failed to create\n");
  }
  *parser_p = parser;
  if (parsers_add(handle1, parser_p)) {
    printf("Failed to add\n");
  }
  list_show(handle1, NULL, parsers_show);
  if (parser_p != list_entry_payload(list_next(handle1, NULL))) {
    printf("Parsers differ\n");
  }
  if (parsers_new(&handle2)) {
    printf("Failed to create\n");
  }
  list_show(handle2, NULL, parsers_show);
  list_show(handle1, NULL, parsers_show);
  parsers_free(handle2);
  parsers_free(handle1);
  return 0;
}
