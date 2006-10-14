/* Herve Fache

20061006 Creation
*/

#include "list.h"
#include "filters.h"
#include "filelist.h"
#include "cvs_parser.h"
#include "db.h"

/* List of files */
list_t file_list;

void file_data_show(const void *payload, char *string) {
  const filelist_data_t *filedata = payload;

  sprintf(string, "%s", filedata->path);
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
  filters_add(handle, ".svn", filter_file_start);

  filelist_new("test////", handle);
  list_show(filelist_get(), NULL, file_data_show);

  db_open("test_db");
  db_parse("file://host/share", filelist_get());
  db_close();

  filelist_free();
  filters_free(handle);
  parsers_free();
  return 0;
}
