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
  const filedata_t *filedata = payload;

  sprintf(string, "%s", filedata->path);
}

int main(int argc, char **argv) {
  void *filters_handle = NULL;
  void *parsers_handle = NULL;

  if (argc == 2) {
    printf("%s\n", argv[1]);
  }
  parsers_new(&parsers_handle);
  parsers_add(parsers_handle, cvs_parser_new());

  filters_new(&filters_handle);
  filters_add(filters_handle, "test/subdir", filter_path_start);
  filters_add(filters_handle, "~", filter_end);
  filters_add(filters_handle, "\\.o$", filter_file_regexp);
  filters_add(filters_handle, ".svn", filter_file_start);

  filelist_new("test////", filters_handle, parsers_handle);
  list_show(filelist_getlist(), NULL, file_data_show);

  db_open("test_db");
  db_parse("file://host/share", filelist_getpath(), filelist_getlist());
  db_close();

  filelist_free();
  filters_free(filters_handle);
  parsers_free(parsers_handle);
  return 0;
}
