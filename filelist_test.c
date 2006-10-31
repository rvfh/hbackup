/* Herve Fache

20061003 Creation
*/

/* Yes, include C file */
#include "filelist.c"
#include "cvs_parser.h"

int verbosity(void) {
  return 0;
}

int terminating(void) {
  return 0;
}

int main(int argc, char *argv[]) {
  void *test_filters_handle = NULL;
  void *test_parsers_handle = NULL;
  int status;

  if ((status = filters_new(&test_filters_handle))) {
    printf("filters_new error status %u\n", status);
  }
  /* Make sure filelist_new uses the filters */
  filters_handle = test_filters_handle;
  if ((status = parsers_new(&test_parsers_handle))) {
    printf("parsers_new error status %u\n", status);
  }
  parsers_handle = test_parsers_handle;
  mount_path_length = 4;

  printf("iterate_directory\n");
  files = list_new(filedata_get);
  if (! iterate_directory("test", NULL)) {
    list_show(files, NULL, NULL);
  }
  list_free(files);

  printf("as previous with subdir in ignore list\n");
  if ((status = filters_add(test_filters_handle, "subdir", filter_path_start))) {
    printf("ignore_add error status %u\n", status);
  }
  files = list_new(filedata_get);
  if (! iterate_directory("test", NULL)) {
    list_show(files, NULL, NULL);
  }
  list_free(files);

  printf("as previous with testlink in ignore list\n");
  if ((status = filters_add(test_filters_handle, "testlink", filter_path_start))) {
    printf("ignore_add error status %u\n", status);
  }
  files = list_new(filedata_get);
  if (! iterate_directory("test", NULL)) {
    list_show(files, NULL, NULL);
  }
  list_free(files);

  printf("as previous with CVS parser\n");
  parsers_add(test_parsers_handle, cvs_parser_new());
  files = list_new(filedata_get);
  if (! iterate_directory("test", NULL)) {
    list_show(files, NULL, NULL);
  }
  list_free(files);

  /* Now make sure we don't mess with private data */
  filters_handle = NULL;
  parsers_handle = NULL;
  mount_path_length = 0;

  printf("filelist_new, as previous\n");
  if (! filelist_new("test", test_filters_handle, test_parsers_handle)) {
    list_show(files, NULL, NULL);
    filelist_free();
  }

  filters_free(test_filters_handle);
  parsers_free(test_parsers_handle);

  return 0;
}
