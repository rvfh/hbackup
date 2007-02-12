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

/* Create a list of files to be backed up

Algorithm for temporary list creation:
 is directory under known SCM control
  yes: is file reported added or modified
   yes: add to temporary list of files to be backed up
   no: nothing
  no: for each element of current directory
   is it to be ignored?
    yes: nothing
    no: is it a directory?
     yes: add to temporary list of files to be backed up and descend into it
     no: add to temporary list of files to be backed up
*/

using namespace std;

#include <iostream>
#include <string>
#include <sys/types.h>
#include <dirent.h>
#include "list.h"
#include "metadata.h"
#include "common.h"
#include "filters.h"
#include "parsers.h"
#include "tools.h"
#include "filelist.h"

static char *filedata_get(const void *payload) {
  const filedata_t *filedata = (const filedata_t *) (payload);
  char *string = NULL;

  asprintf(&string, "%s", filedata->path.c_str());
  return string;
}

int FileList::iterate_directory(const string& path, parser_t *parser) {
  void          *handle;
  filedata_t    filedata;
  filedata_t    *filedata_p;
  DIR           *directory;
  struct dirent *dir_entry;

  if (verbosity() > 3) {
    cout << " --> Dir: " << path.substr(_mount_path_length) << endl;
  }
  /* Check whether directory is under SCM control */
  if (parsers_dir_check(_parsers_handle, &parser, &handle, path.c_str()) == 2) {
    return 0;
  }
  directory = opendir(path.c_str());
  if (directory == NULL) {
    cerr << "filelist: cannot open directory: " << path << endl;
    return 2;
  }
  while (((dir_entry = readdir(directory)) != NULL) && ! terminating()) {
    string  file_path;
    int     failed = 0;

    /* Ignore . and .. */
    if (! strcmp(dir_entry->d_name, ".") || ! strcmp(dir_entry->d_name, "..")){
      continue;
    }
    file_path = path + "/" + dir_entry->d_name;
    /* Remove mount path and leading slash from records */
    filedata.path = file_path.substr(_mount_path_length + 1);
    strcpy(filedata.checksum, "");
    if (metadata_get(file_path.c_str(), &filedata.metadata)) {
      cerr << "filelist: cannot get metadata: " << file_path << endl;
      failed = 2;
    } else
    /* Let the parser analyse the file data to know whether to back it up */
    if (parsers_file_check(parser, handle, &filedata)) {
      failed = 1;
    } else
    /* Now pass it through the filters */
    if ((_filter_handle != NULL) && ! _filter_handle->match(&filedata)) {
      failed = 1;
    } else
    if (S_ISDIR(filedata.metadata.type)) {
      filedata.metadata.size = 0;
      if (iterate_directory(file_path, parser)) {
        if (! terminating()) {
          cerr << "filelist: cannot iterate into directory: "
            << dir_entry->d_name << endl;
        }
        failed = 2;
      }
    }
    if (! failed) {
      filedata_p = new filedata_t;
      *filedata_p = filedata;
      _files->add(filedata_p);
    }
  }
  closedir(directory);
  if (terminating()) {
    return 1;
  }
  return 0;
}

FileList::FileList(
    const string& mount_path,
    const Filter  *filter,
    const List    *parsers) {
  _filter_handle = filter;
  _parsers_handle = parsers;
  _files = new List(filedata_get);
  _mount_path_length = mount_path.size();
  if (iterate_directory(mount_path, NULL)) {
    delete _files;
    _files = NULL;
  }
}

List *FileList::getList() {
  return _files;
}
