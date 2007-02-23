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
#include <list>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

#include "files.h"
#include "filters.h"
#include "parsers.h"
#include "cvs_parser.h"
#include "paths.h"
#include "hbackup.h"

int Path::iterate_directory(const string& path, Parser* parser) {
  if (verbosity() > 3) {
    cout << " ---> Dir: " << path.substr(_mount_path_length) << endl;
  }
  /* Check whether directory is under SCM control */
  if (_parsers.empty()) {
    parser = NULL;
  } else {
    // We have a parser, check this directory with it
    if (parser != NULL) {
      parser = parser->isControlled(path);
    }
    // We don't have a parser [anymore], check this directory
    if (parser == NULL) {
      parser = _parsers.isControlled(path);
    }
  }
  DIR* directory = opendir(path.c_str());
  if (directory == NULL) {
    cerr << "paths: cannot open directory: " << path << endl;
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
      cerr << "paths: cannot get metadata: " << file_path << endl;
      continue;
    } else
    /* Let the parser analyse the file data to know whether to back it up */
    if ((parser != NULL) && (parser->ignore(file_data))) {
      continue;
    } else
    /* Now pass it through the filters */
    if (! _filters.empty() && ! _filters.match(file_data)) {
      continue;
    } else
    if (S_ISDIR(file_data.type())) {
      if (iterate_directory(file_path, parser)) {
        if (! terminating()) {
          cerr << "paths: cannot iterate into directory: "
            << dir_entry->d_name << endl;
        }
        continue;
      }
    }
    _list.push_back(file_data);
  }
  closedir(directory);
  if (terminating()) {
    return 1;
  }
  return 0;
}

Path::Path(const string& path) {
  _path = path;

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
    const string& type,
    const string& value,
    bool          append) {
  Condition *condition;

  if (append && _filters.empty()) {
    // Can't append to nothing
    return 3;
  }

  /* Add specified filter */
  if (type == "type") {
    mode_t file_type;
    if (value == "file") {
      file_type = S_IFREG;
    } else
    if (value == "dir") {
      file_type = S_IFDIR;
    } else
    if (value == "char") {
      file_type = S_IFCHR;
    } else
    if (value == "block") {
      file_type = S_IFBLK;
    } else
    if (value == "pipe") {
      file_type = S_IFIFO;
    } else
    if (value == "link") {
      file_type = S_IFLNK;
    } else
    if (value == "socket") {
      file_type = S_IFSOCK;
    } else {
      // Wrong value
      return 2;
    }
    condition = new Condition(filter_type, file_type);
  } else
  if (type == "path_end") {
    condition = new Condition(filter_path_end, value);
  } else
  if (type == "path_start") {
    condition = new Condition(filter_path_start, value);
  } else
  if (type == "path_regexp") {
    condition = new Condition(filter_path_regexp, value);
  } else
  if (type == "size_below") {
    off_t size = strtoul(value.c_str(), NULL, 10);
    condition = new Condition(filter_size_below, size);
  } else
  if (type == "size_above") {
    off_t size = strtoul(value.c_str(), NULL, 10);
    condition = new Condition(filter_size_above, size);
  } else {
    // Wrong type
    return 1;
  }

  if (append) {
    _filters[_filters.size() - 1].push_back(*condition);
  } else {
    _filters.push_back(Filter(*condition));
  }
  delete condition;
  return 0;
}

int Path::addParser(
    const string& type,
    const string& string) {
  parser_mode_t mode;

  /* Determine mode */
  switch (type[0]) {
    case 'c':
      // All controlled files
      mode = parser_controlled;
      break;
    case 'l':
      // Local files
      mode = parser_modifiedandothers;
      break;
    case 'm':
      // Modified controlled files
      mode = parser_modified;
      break;
    case 'o':
      // Non controlled files
      mode = parser_others;
      break;
    default:
      cerr << "Undefined mode " << type << " for parser " << string << endl;
      return 1;
  }

  /* Add specified parser */
  if (string == "cvs") {
    _parsers.push_back(new CvsParser(mode));
  } else {
    cerr << "Unsupported parser " << string << endl;
    return 1;
  }
  return 0;
}

int Path::createList(const string& backup_path) {
  _mount_path_length = backup_path.size();
  _list.clear();
  if (iterate_directory(backup_path, NULL)) {
    return 1;
  }
  _list.sort();
  return 0;
}
