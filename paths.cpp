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

using namespace std;

#include <iostream>
#include <string>
#include <vector>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

#include "list.h"
#include "files.h"
#include "filters.h"
#include "parser.h"
#include "parsers.h"
#include "cvs_parser.h"
#include "paths.h"
#include "hbackup.h"

static char *filedata_get(const void* payload) {
  char *string = NULL;

  asprintf(&string, "%s", ((File *) payload)->path().c_str());
  return string;
}

int Path::iterate_directory(const string& path, Parser* parser) {
  if (verbosity() > 3) {
    cout << " ---> Dir: " << path.substr(_mount_path_length) << endl;
  }
  /* Check whether directory is under SCM control */
  if (_parsers.size() != 0) {
    // We have a parser, check this directory with it
    if (parser != NULL) {
      parser = parser->isControlled(path);
    }
    // We don't have a parser [anymore], check this directory
    if (parser == NULL) {
      parser = _parsers.isControlled(path);
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
    /* Ignore . and .. */
    if (! strcmp(dir_entry->d_name, ".") || ! strcmp(dir_entry->d_name, "..")){
      continue;
    }
    string file_path = path + "/" + dir_entry->d_name;
    File   file_data(file_path.substr(0, _mount_path_length),
      file_path.substr(_mount_path_length + 1));

    /* Remove mount path and leading slash from records */
    if (file_data.type() == 0) {
      cerr << "filelist: cannot get metadata: " << file_path << endl;
      continue;
    } else
    /* Let the parser analyse the file data to know whether to back it up */
    if ((parser != NULL) && (parser->ignore(file_data))) {
      continue;
    } else
    /* Now pass it through the filters */
    if ((_filters.size() != 0) && ! _filters.match(file_data)) {
      continue;
    } else
    if (S_ISDIR(file_data.type())) {
      if (iterate_directory(file_path, parser)) {
        if (! terminating()) {
          cerr << "filelist: cannot iterate into directory: "
            << dir_entry->d_name << endl;
        }
        continue;
      }
    }
    _list->add(new File(file_data));
  }
  closedir(directory);
  if (terminating()) {
    return 1;
  }
  return 0;
}

Path::Path(const string& path) {
  _path = path;
  _list = NULL;

  unsigned int pos = 0;
  while ((pos = _path.find("\\", pos)) != string::npos) {
    _path.replace(pos, 1, "/");
  }
  pos = _path.size() - 1;
  while (_path[pos] == '/') {
    pos--;
  }
  if (pos < _path.size() - 1) {
    _path.erase(pos + 1);
  }
}

int Path::addFilter(
    const string& in_type,
    const string& string) {
  const char *type = in_type.c_str();
  const char *filter_type;
  const char *delim    = strchr(type, '/');
  mode_t     file_type = 0;

  /* Check whether file type was specified */
  if (delim != NULL) {
    filter_type = delim + 1;
    if (! strncmp(type, "file", delim - type)) {
      file_type = S_IFREG;
    } else if (! strncmp(type, "dir", delim - type)) {
      file_type = S_IFDIR;
    } else if (! strncmp(type, "char", delim - type)) {
      file_type = S_IFCHR;
    } else if (! strncmp(type, "block", delim - type)) {
      file_type = S_IFBLK;
    } else if (! strncmp(type, "pipe", delim - type)) {
      file_type = S_IFIFO;
    } else if (! strncmp(type, "link", delim - type)) {
      file_type = S_IFLNK;
    } else if (! strncmp(type, "socket", delim - type)) {
      file_type = S_IFSOCK;
    } else {
      return 1;
    }
  } else {
    filter_type = type;
    file_type = S_IFMT;
  }

  /* Add specified filter */
  if (! strcmp(filter_type, "path_end")) {
    _filters.push_back(new Filter(new Condition(file_type, filter_path_end, string.c_str())));
  } else if (! strcmp(filter_type, "path_start")) {
    _filters.push_back(new Filter(new Condition(file_type, filter_path_start, string.c_str())));
  } else if (! strcmp(filter_type, "path_regexp")) {
    _filters.push_back(new Filter(new Condition(file_type, filter_path_regexp, string.c_str())));
  } else if (! strcmp(filter_type, "size_below")) {
    _filters.push_back(new Filter(new Condition(0, filter_size_below, strtoul(string.c_str(), NULL, 10))));
  } else if (! strcmp(filter_type, "size_above")) {
    _filters.push_back(new Filter(new Condition(0, filter_size_above, strtoul(string.c_str(), NULL, 10))));
  } else {
    return 1;
  }
  return 0;
}

int Path::addParser(
    const string& type,
    const string& string) {
  parser_mode_t mode;

  /* Determine mode */
  if (type == "mod") {
    mode = parser_modified;
  } else
  if (type == "mod+oth") {
    mode = parser_modifiedandothers;
  } else
  if (type == "oth") {
    mode = parser_others;
  } else {
    /* Default */
    mode = parser_controlled;
  }

  /* Add specified parser */
  if (string == "cvs") {
    _parsers.push_back(new CvsParser(mode));
  } else {
    return 1;
  }
  return 0;
}

int Path::backup(const string& backup_path) {
  _list = new List(filedata_get);
  _mount_path_length = backup_path.size();
  if (iterate_directory(backup_path, NULL)) {
    delete _list;
    _list = NULL;
    return 1;
  }
  return 0;
}