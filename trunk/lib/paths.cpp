/*
     Copyright (C) 2006-2007  Herve Fache

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

#include <iostream>
#include <string>
#include <list>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

using namespace std;

#include "list.h"
#include "files.h"
#include "filters.h"
#include "parsers.h"
#include "cvs_parser.h"
#include "paths.h"
#include "hbackup.h"

using namespace hbackup;

int Path::iterate_directory(const string& path, Parser* parser) {
  /* Check whether directory is under SCM control */
  if (! _parsers.empty()) {
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
    cerr << strerror(errno) << ": " << path << endl;
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
    if (file_data.type() == '?') {
      cerr << "paths: cannot get metadata: " << file_path << endl;
      continue;
    } else
    /* Let the parser analyse the file data to know whether to back it up */
    if ((parser != NULL) && (parser->ignore(file_data))) {
      continue;
    } else
    /* Now pass it through the filters */
    if (! _filters.empty() && _filters.match(file_data)) {
      continue;
    } else
    if (file_data.type() == 'd') {
      if (verbosity() > 3) {
        cout << " ---> Dir: " << file_path.substr(_mount_path_length) << endl;
      }
      if (iterate_directory(file_path, parser)) {
        continue;
      }
    }
    _list.add(file_data);
  }
  closedir(directory);
  if (terminating()) {
    return 1;
  }
  return 0;
}

Path::Path(const string& path) {
  _path       = path;
  _expiration = 0;

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
    char file_type;
    if ((value == "file") || (value == "f")) {
      file_type = 'f';
    } else
    if ((value == "dir") || (value == "d")) {
      file_type = 'd';
    } else
    if ((value == "char") || (value == "c")) {
      file_type = 'c';
    } else
    if ((value == "block") || (value == "b")) {
      file_type = 'b';
    } else
    if ((value == "pipe") || (value == "p")) {
      file_type = 'p';
    } else
    if ((value == "link") || (value == "l")) {
      file_type = 'l';
    } else
    if ((value == "socket") || (value == "s")) {
      file_type = 's';
    } else {
      // Wrong value
      return 2;
    }
    condition = new Condition(filter_type, file_type);
  } else
  if (type == "name") {
    condition = new Condition(filter_name, value);
  } else
  if (type == "name_start") {
    condition = new Condition(filter_name_start, value);
  } else
  if (type == "name_end") {
    condition = new Condition(filter_name_end, value);
  } else
  if (type == "name_regex") {
    condition = new Condition(filter_name_regex, value);
  } else
  if (type == "path") {
    condition = new Condition(filter_path, value);
  } else
  if (type == "path_start") {
    condition = new Condition(filter_path_start, value);
  } else
  if (type == "path_end") {
    condition = new Condition(filter_path_end, value);
  } else
  if (type == "path_regex") {
    condition = new Condition(filter_path_regex, value);
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
    _filters.back().push_back(*condition);
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
  switch (string[0]) {
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
  if (type == "cvs") {
    _parsers.push_back(new CvsParser(mode));
  } else {
    cerr << "Unsupported parser " << string << endl;
    return 2;
  }
  return 0;
}

int Path::createList(const string& backup_path) {
  _mount_path_length = backup_path.size();
  _list.clear();
  if (iterate_directory(backup_path, NULL)) {
    _list.clear();
    return 1;
  }
  return 0;
}

int Path2::recurse(const char* path, Directory* dir, Parser* parser) {
  if (terminating()) {
    errno = EINTR;
    return -1;
  }
  /* Check whether directory is under SCM control */
  if (! _parsers.empty()) {
    // We have a parser, check this directory with it
    if (parser != NULL) {
      parser = parser->isControlled(dir->path());
    }
    // We don't have a parser [anymore], check this directory
    if (parser == NULL) {
      parser = _parsers.isControlled(dir->path());
    }
  }
  if (dir->isValid() && ! dir->createList()) {
    list<Node*>::iterator i = dir->nodesList().begin();
    while (i != dir->nodesList().end()) {
      Node* node = *i;

      // Ignore inaccessible files
      if (node->type() == '?') {
        i = dir->nodesList().erase(i);
        continue;
      }

      /* Let the parser analyse the file data to know whether to back it up */
      if ((parser != NULL) && (parser->ignore(*node))) {
        i = dir->nodesList().erase(i);
        continue;
      }

      /* Now pass it through the filters */
      if (! _filters.empty() && _filters.match(path, *node)) {
        i = dir->nodesList().erase(i);
        continue;
      }

      switch (node->type()) {
        case 'f': {
          File2 *f = new File2(*node);
          delete *i;
          *i = f;
        }
        break;
        case 'l': {
          Link *l = new Link(*node);
          delete *i;
          *i = l;
        }
        break;
        case 'd': {
          Directory *d = new Directory(*node);
          delete *i;
          *i = d;
          char* dir_path = NULL;
          asprintf(&dir_path, "%s%s/", path, node->name());
          if (verbosity() > 3) {
            cout << " ---> Dir: " << dir_path << endl;
          }
          recurse(dir_path, d, parser);
          free(dir_path);
        }
        break;
      }
      i++;
    }
  }
  return 0;
}

Path2::Path2(const char* path) {
  _path              = NULL;
  _backup_path       = NULL;
  _expiration        = 0;
  _mount_path_length = 0;
  char* current;

  // Copy path accross
  asprintf(&_path, "%s", path);

  // Change '\' into '/'
  current = _path;
  while (current < &_path[strlen(_path)]) {
    if (*current == '\\') {
      *current = '/';
    }
    current++;
  }

  // Remove trailing '/'s
  while ((--current >= _path) && (*current == '/')) {
    *current = '\0';
  }
}

int Path2::addFilter(
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
    char file_type;
    if ((value == "file") || (value == "f")) {
      file_type = 'f';
    } else
    if ((value == "dir") || (value == "d")) {
      file_type = 'd';
    } else
    if ((value == "char") || (value == "c")) {
      file_type = 'c';
    } else
    if ((value == "block") || (value == "b")) {
      file_type = 'b';
    } else
    if ((value == "pipe") || (value == "p")) {
      file_type = 'p';
    } else
    if ((value == "link") || (value == "l")) {
      file_type = 'l';
    } else
    if ((value == "socket") || (value == "s")) {
      file_type = 's';
    } else {
      // Wrong value
      return 2;
    }
    condition = new Condition(filter_type, file_type);
  } else
  if (type == "name") {
    condition = new Condition(filter_name, value);
  } else
  if (type == "name_start") {
    condition = new Condition(filter_name_start, value);
  } else
  if (type == "name_end") {
    condition = new Condition(filter_name_end, value);
  } else
  if (type == "name_regex") {
    condition = new Condition(filter_name_regex, value);
  } else
  if (type == "path") {
    condition = new Condition(filter_path, value);
  } else
  if (type == "path_start") {
    condition = new Condition(filter_path_start, value);
  } else
  if (type == "path_end") {
    condition = new Condition(filter_path_end, value);
  } else
  if (type == "path_regex") {
    condition = new Condition(filter_path_regex, value);
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
    _filters.back().push_back(*condition);
  } else {
    _filters.push_back(Filter(*condition));
  }
  delete condition;
  return 0;
}

int Path2::addParser(
    const string& type,
    const string& string) {
  parser_mode_t mode;

  /* Determine mode */
  switch (string[0]) {
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
  if (type == "cvs") {
    _parsers.push_back(new CvsParser(mode));
  } else {
    cerr << "Unsupported parser " << string << endl;
    return 2;
  }
  return 0;
}

int Path2::parse(const char* backup_path) {
  int rc = 0;
  asprintf(&_backup_path, "%s/", backup_path);
  _dir = new Directory(backup_path);
  if (recurse("", _dir, NULL)) {
    delete _dir;
    _dir = NULL;
    rc = -1;
  }
  free(_backup_path);
  _backup_path = NULL;
  return rc;
}
