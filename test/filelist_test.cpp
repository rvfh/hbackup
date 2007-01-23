/*
     Copyright (C) 2006  Herve Fache

     This program is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License version 2 as
     published by the Free Software Foundation.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place - Suite 330,
     Boston, MA 02111-1307, USA.
*/

#include "filelist.cpp"
#include "cvs_parser.h"

int verbosity(void) {
  return 0;
}

int terminating(void) {
  return 0;
}

int main(int argc, char *argv[]) {
  List *test_filters_handle = NULL;
  List *test_parsers_handle = NULL;
  int status;

  if ((status = filters_new(&test_filters_handle))) {
    printf("filters_new error status %u\n", status);
  }
  /* Make sure filenew List uses the filters */
  filters_handle = test_filters_handle;
  if ((status = parsers_new(&test_parsers_handle))) {
    printf("parsers_new error status %u\n", status);
  }
  parsers_handle = test_parsers_handle;
  mount_path_length = 4;

  printf("iterate_directory\n");
  files = new List(filedata_get);
  if (! iterate_directory("test", NULL)) {
    cout << ">List " << files->size() << " file(s):\n";
    files->show();
  }
  delete files;

  printf("as previous with subdir in ignore list\n");
  if ((status = filters_rule_add(filters_rule_new(test_filters_handle), S_IFDIR, filter_path_start, "subdir"))) {
    printf("ignore_add error status %u\n", status);
  }
  files = new List(filedata_get);
  if (! iterate_directory("test", NULL)) {
    cout << ">List " << files->size() << " file(s):\n";
    files->show();
  }
  delete files;

  printf("as previous with testlink in ignore list\n");
  if ((status = filters_rule_add(filters_rule_new(test_filters_handle), S_IFLNK, filter_path_start, "testlink"))) {
    printf("ignore_add error status %u\n", status);
  }
  files = new List(filedata_get);
  if (! iterate_directory("test", NULL)) {
    cout << ">List " << files->size() << " file(s):\n";
    files->show();
  }
  delete files;

  printf("as previous with CVS parser\n");
  parsers_add(test_parsers_handle, parser_controlled, cvs_parser_new());
  files = new List(filedata_get);
  if (! iterate_directory("test", NULL)) {
    cout << ">List " << files->size() << " file(s):\n";
    files->show();
  }
  delete files;

  /* Now make sure we don't mess with private data */
  filters_handle = NULL;
  parsers_handle = NULL;
  mount_path_length = 0;

  printf("filelist_new, as previous\n");
  if (! filelist_new("test", test_filters_handle, test_parsers_handle)) {
    cout << ">List " << files->size() << " file(s):\n";
    files->show();
    filelist_free();
  }

  filters_free(test_filters_handle);
  parsers_free(test_parsers_handle);

  return 0;
}
