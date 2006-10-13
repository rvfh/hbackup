/* Herve Fache

20061006 Creation
*/

#include "list.h"
#include "filters.h"
#include "filelist.h"
#include "cvs_parser.h"

/* List of files */
list_t file_list;

void file_data_show(const void *payload_p, char *string) {
  const metadata_t *metadata = payload_p;

  sprintf(string, "%s", metadata->path);
}

int main(int argc, char **argv) {
  void *handle = NULL;

  if (argc == 2) {
    printf("%s\n", argv[1]);
  }
  parsers_new();
  parsers_add(cvs_parser_new());

  filters_new(&handle);
  filters_add(handle, "test/subdir", filter_path_start);
  filters_add(handle, "~", filter_end);
  filters_add(handle, "\\.o$", filter_file_regexp);

  file_list_new("test////", handle);

  list_show(file_list_get(), NULL, file_data_show);

  file_list_free();
  filters_free(handle);
  parsers_free();
  return 0;
}
