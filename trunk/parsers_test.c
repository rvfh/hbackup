/* Herve Fache

20061005 Creation
*/

#include <stdio.h>
#include "parsers.c"

static void destroy(void) {
  printf("destroy called\n");
}

static parser_t parser = { (parsers_dir_check_f *) 0x12345678, (parsers_dir_leave_f *) 0xDEADBEEF, (parsers_file_check_f *) 0x34567890, destroy };

/* Use payload as argument name, cast once and for all */
static void parsers_show(const void *payload, char *string) {
  const parser_t *parser = payload;

  sprintf(string, "[0x%08x, 0x%08x, 0x%08x]", (unsigned int) parser->dir_check,
      (unsigned int) parser->dir_leave, (unsigned int) parser->file_check);
}

int main(void) {
  parser_t *parser_p = malloc(sizeof(parser_t));
  void *handle = NULL;

  if (parsers_new()) {
    printf("Failed to create\n");
  }
  *parser_p = parser;
  if (parsers_add(parser_p)) {
    printf("Failed to add\n");
  }
  list_show(parsers_list, NULL, parsers_show);
  if (parser_p != parsers_next(&handle)) {
    printf("Parsers differ\n");
  }
  parsers_free();
  return 0;
}
