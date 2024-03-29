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

#include "strings.h"
#include "files.h"
#include "filters.h"
#include "parsers.h"
#include "cvs_parser.h"
#include "dbdata.h"
#include "list.h"
#include "db.h"
#include "paths.h"
#include "hbackup.h"

using namespace hbackup;

int Path::recurse(
    Database&   db,
    const char* prefix,
    const char* cur_path,
    Directory*  dir,
    Parser*     parser) {
  if (terminating()) {
    errno = EINTR;
    return -1;
  }

  // Get relative path
  const char* rel_path;
  if (cur_path[_backup_path_length] == '\0') {
    rel_path = &cur_path[_backup_path_length];
  } else {
    rel_path = &cur_path[_backup_path_length + 1];
  }

  // Check whether directory is under SCM control
  if (! _parsers.empty()) {
    // We have a parser, check this directory with it
    if (parser != NULL) {
      parser = parser->isControlled(cur_path);
    }
    // We don't have a parser [anymore], check this directory
    if (parser == NULL) {
      parser = _parsers.isControlled(cur_path);
    }
  }
  if (dir->isValid() && ! dir->createList(cur_path)) {
    list<Node*> db_list;
    // Get database info for this directory
    db.getList(prefix, _path.c_str(), rel_path, db_list);

    list<Node*>::iterator i = dir->nodesList().begin();
    list<Node*>::iterator j = db_list.begin();
    while (i != dir->nodesList().end()) {
      if (! terminating()) {
        // Ignore inaccessible files
        if ((*i)->type() == '?') {
          i = dir->nodesList().erase(i);
          continue;
        }

        // Let the parser analyse the file data to know whether to back it up
        if ((parser != NULL) && (parser->ignore(*(*i)))) {
          i = dir->nodesList().erase(i);
          continue;
        }

        // Now pass it through the filters
        if (! _filters.empty() && _filters.match(rel_path, *(*i))) {
          i = dir->nodesList().erase(i);
          continue;
        }

        // Count the nodes considered, for info
        _nodes++;

        // For link, find out linked path
        if ((*i)->type() == 'l') {
          Link *l = new Link(*(*i), cur_path);
          delete *i;
          *i = l;
        }

        // Also deal with directory, as some fields should not be considered
        if ((*i)->type() == 'd') {
          Directory *d = new Directory(*(*i));
          delete *i;
          *i = d;
        }

        // Synchronize with DB records
        int cmp = -1;
        while ((j != db_list.end())
            && ((cmp = Node::pathCompare((*j)->name(), (*i)->name())) < 0)) {
          if (! terminating()) {
            if (verbosity() > 2) {
              cout << " --> R ";
              if (rel_path[0] != '\0') {
                cout << rel_path << "/";
              }
              cout << (*j)->name() << endl;
            }
            recurse_remove(db, prefix, _path, rel_path, *j);
          }
          delete *j;
          j = db_list.erase(j);
        }

        // Deal with data
        if ((j == db_list.end()) || (cmp > 0)) {
          // Not found in DB => new
          if (verbosity() > 2) {
            cout << " --> A ";
            if (rel_path[0] != '\0') {
              cout << rel_path << "/";
            }
            cout << (*i)->name() << endl;
          }
          db.add(prefix, _path.c_str(), rel_path, cur_path, *i);
        } else {
          // Same file name found in DB
          if (**i != **j) {
            const char* checksum = NULL;
            // Metadata differ
            if (((*i)->type() == 'f')
            && ((*j)->type() == 'f')
            && ((*i)->size() == (*j)->size())
            && ((*i)->mtime() == (*j)->mtime())) {
              // If the file data is there, just add new metadata
              // If the checksum is missing, this shall retry too
              checksum = ((File*)(*j))->checksum();
              if (verbosity() > 2) {
                cout << " --> ~ ";
              }
            } else {
              // Do it all
              if (verbosity() > 2) {
                cout << " --> M ";
              }
            }
            if (verbosity() > 2) {
              if (rel_path[0] != '\0') {
                cout << rel_path << "/";
              }
              cout << (*i)->name() << endl;
            }
            db.add(prefix, _path.c_str(), rel_path, cur_path, *i, checksum);
          } else {
            // i and j have same metadata, hence same type...
            // Compare linked data
            if (((*i)->type() == 'l')
            && (strcmp(((Link*)(*i))->link(), ((Link*)(*j))->link()) != 0)) {
              if (verbosity() > 2) {
                cout << " --> L ";
                if (rel_path[0] != '\0') {
                  cout << rel_path << "/";
                }
                cout << (*i)->name() << endl;
              }
              db.add(prefix, _path.c_str(), rel_path, cur_path, *i);
            } else
            // Check that file data is present
            if (((*i)->type() == 'f')
             && (((File*)(*j))->checksum()[0] == '\0')) {
              // Checksum missing: retry
              if (verbosity() > 2) {
                cout << " --> ! ";
                if (rel_path[0] != '\0') {
                  cout << rel_path << "/";
                }
                cout << (*i)->name() << endl;
              }
              const char* checksum = ((File*)(*j))->checksum();
              db.add(prefix, _path.c_str(), rel_path, cur_path, *i, checksum);
            } else if ((*i)->type() == 'd') {
              if (verbosity() > 3) {
                cout << " --> D ";
                if (rel_path[0] != '\0') {
                  cout << rel_path << "/";
                }
                cout << (*i)->name() << endl;
              }
            }
          }
          delete *j;
          j = db_list.erase(j);
        }

        // For directory, recurse into it
        if ((*i)->type() == 'd') {
          char* dir_path = Node::path(cur_path, (*i)->name());
          recurse(db, prefix, dir_path, (Directory*) *i, parser);
          free(dir_path);
        }
      }
      delete *i;
      i = dir->nodesList().erase(i);
    }

    // Deal with removed records
    while (j != db_list.end()) {
      if (! terminating()) {
        if (verbosity() > 2) {
          cout << " --> R ";
          if (rel_path[0] != '\0') {
            cout << rel_path << "/";
          }
          cout << (*j)->name() << endl;
        }
        recurse_remove(db, prefix, _path, rel_path, *j);
      }
      delete *j;
      j = db_list.erase(j);
    }
  } else {
    cerr << strerror(errno) << ": " << rel_path << endl;
  }
  return 0;
}

void Path::recurse_remove(
    Database&     db,
    const char*   prefix,
    const StrPath base_path,
    const char*   rel_path,
    const Node*   node) {
  db.remove(prefix, base_path.c_str(), rel_path, node);
  // Recurse into directories
  if (node->type() == 'd') {
    list<Node*> db_list;
    char* dir_path = Node::path(rel_path, node->name());

    // Get database info for this directory
    db.getList(prefix, base_path.c_str(), dir_path, db_list);
    list<Node*>::iterator j = db_list.begin();
    while (j != db_list.end()) {
      recurse_remove(db, prefix, base_path.c_str(), dir_path, *j);
      delete *j;
      j = db_list.erase(j);
    }
    free(dir_path);
  }
}

Path::Path(const char* path) {
  _dir                = NULL;
  _expiration         = 0;
  _backup_path_length = 0;

  // Copy path accross
  _path = path;

  // Change '\' into '/'
  _path.toUnix();

  // Remove trailing '/'s
  _path.noEndingSlash();
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

int Path::parse(
    Database&   db,
    const char* prefix,
    const char* backup_path) {
  int rc = 0;
  _backup_path_length = strlen(backup_path);
  _nodes = 0;
  _dir = new Directory(backup_path);
  if (recurse(db, prefix, backup_path, _dir, NULL)) {
    delete _dir;
    _dir = NULL;
    rc = -1;
  }
  return rc;
}
