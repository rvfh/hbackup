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
  Filter  *test_filter_handle = NULL;
  List    *test_parsers_handle = NULL;
  int     status;

  test_filter_handle = new Filter;
  /* Make sure filenew List uses the filters */
  if ((status = parsers_new(&test_parsers_handle))) {
    cout << "parsers_new error status " << status << endl;
  }

  FileList file_list1("test", test_filter_handle, test_parsers_handle);
  if (file_list1.getList() != NULL) {
    cout << ">List " << file_list1.getList()->size() << " file(s):\n";
    file_list1.getList()->show();
  }

  cout << "as previous with subdir in ignore list" << endl;
  test_filter_handle->push_back(new Rule(new Condition(S_IFDIR,
    filter_path_start, "subdir")));
  FileList file_list2("test", test_filter_handle, test_parsers_handle);
  if (file_list2.getList() != NULL) {
    cout << ">List " << file_list2.getList()->size() << " file(s):\n";
    file_list2.getList()->show();
  }

  cout << "as previous with testlink in ignore list" << endl;
  test_filter_handle->push_back(new Rule(new Condition(S_IFLNK, filter_path_start, "testlink")));
  FileList file_list3("test", test_filter_handle, test_parsers_handle);
  if (file_list3.getList() != NULL) {
    cout << ">List " << file_list3.getList()->size() << " file(s):\n";
    file_list3.getList()->show();
  }

  cout << "as previous with CVS parser" << endl;
  parsers_add(test_parsers_handle, parser_controlled, cvs_parser_new());
  FileList file_list4("test", test_filter_handle, test_parsers_handle);
  if (file_list4.getList() != NULL) {
    cout << ">List " << file_list4.getList()->size() << " file(s):\n";
    file_list4.getList()->show();
  }

  cout << "as previous" << endl;
  FileList file_list5("test", test_filter_handle, test_parsers_handle);
  if (file_list5.getList() != NULL) {
    cout << ">List " << file_list5.getList()->size() << " file(s):\n";
    file_list5.getList()->show();
  }

  delete test_filter_handle;
  parsers_free(test_parsers_handle);

  return 0;
}
