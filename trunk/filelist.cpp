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
#include "parser.h"
#include "parsers.h"
#include "tools.h"
#include "filelist.h"

static char *filedata_get(const void* payload) {
  const filedata_t* filedata = (const filedata_t*) (payload);
  char*             string   = NULL;

  asprintf(&string, "%s", filedata->path.c_str());
  return string;
}

int FileList::iterate_directory(const string& path, Parser* parser) {
  if (verbosity() > 3) {
    cout << " --> Dir: " << path.substr(_mount_path_length) << endl;
  }
  /* Check whether directory is under SCM control */
  if (_parsers != NULL) {
    if (parser != NULL) {
      // We have a parser, check this directory
      if ((parser = parser->isControlled(path)) == NULL) {
        // Parent is controlled so child should be
        cerr << "filelist: directory should be controlled: " << path << endl;
        parser = new IgnoreParser;
      }
    }
    if (parser == NULL) {
      // We don't have a parser [anymore], check this directory
      parser = _parsers->isControlled(path);
    }
  } else {
    parser = NULL;
  }
  DIR* directory = opendir(path.c_str());
  if (directory == NULL) {
    cerr << "filelist: cannot open directory: " << path << endl;
    return 2;
  }
  struct dirent *dir_entry;
  while (((dir_entry = readdir(directory)) != NULL) && ! terminating()) {
    filedata_t  filedata;
    string      file_path;

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
      continue;
    } else
    /* Let the parser analyse the file data to know whether to back it up */
    if ((parser != NULL) && (parser->ignore(&filedata))) {
      continue;
    } else
    /* Now pass it through the filters */
    if ((_filters != NULL) && ! _filters->match(&filedata)) {
      continue;
    } else
    if (S_ISDIR(filedata.metadata.type)) {
      filedata.metadata.size = 0;
      if (iterate_directory(file_path, parser)) {
        if (! terminating()) {
          cerr << "filelist: cannot iterate into directory: "
            << dir_entry->d_name << endl;
        }
        continue;
      }
    }
    _files->add(new filedata_t(filedata));
  }
  closedir(directory);
  if (terminating()) {
    return 1;
  }
  return 0;
}

FileList::FileList(
    const string&   mount_path,
    const Filter*   filters,
    const Parsers*  parsers) {
  _filters = filters;
  _parsers = parsers;
  _files = new List(filedata_get);
  _mount_path_length = mount_path.size();
  if (iterate_directory(mount_path, NULL)) {
    delete _files;
    _files = NULL;
  }
}

List* FileList::getList() {
  return _files;
}
