/* Herve Fache

20061003 Creation
*/

/* Yes, include C file */
#include "filelist.c"
#include "cvs_parser.h"

int main(int argc, char *argv[]) {
  void *handle;
  int status;

  if ((status = filters_new(&handle))) {
    printf("parsers_new error status %u\n", status);
  }
  /* Make sure file_list uses the filters */
  filters_handle = handle;
  if ((status = parsers_new())) {
    printf("parsers_new error status %u\n", status);
  }

  printf("iterate_directory\n");
  file_list = list_new(file_data_get);
  if (! iterate_directory("test", NULL)) {
    list_show(file_list, NULL, file_data_get);
  }
  list_free(file_list);

  printf("file_list_build\n");
  file_list = list_new(file_data_get);
  if (! file_list_build("test")) {
    list_show(file_list, NULL, file_data_get);
  }
  list_free(file_list);

  printf("as previous with test/subdir in notdir list\n");
  if ((status = filters_add(handle, "test/subdir", filter_path_start))) {
    printf("ignore_add error status %u\n", status);
  }
  file_list = list_new(file_data_get);
  if (! file_list_build("test")) {
    list_show(file_list, NULL, file_data_get);
  }
  list_free(file_list);

  printf("as previous with test/testlink in notdir list\n");
  if ((status = filters_add(handle, "test/testlink", filter_path_start))) {
    printf("ignore_add error status %u\n", status);
  }
  file_list = list_new(file_data_get);
  if (! file_list_build("test")) {
    list_show(file_list, NULL, file_data_get);
  }
  list_free(file_list);

  printf("as previous with CVS parser\n");
  parsers_add(cvs_parser_new());
  file_list = list_new(file_data_get);
  if (! file_list_build("test")) {
    list_show(file_list, NULL, file_data_get);
  }
  list_free(file_list);

  printf("file_list_new, as previous\n");
  if (! file_list_new("test", handle)) {
    list_show(file_list, NULL, file_data_get);
  }

  printf("file_list_new, as previous with ending slash\n");
  if (! file_list_new("test/", handle)) {
    list_show(file_list, NULL, file_data_get);
  }

  filters_free(handle);
  parsers_free();

  return 0;
}
