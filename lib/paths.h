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

#ifndef PATHS_H
#define PATHS_H

#ifndef LIST_H
#error You must include files.h before paths.h
#endif

#ifndef FILES_H
#error You must include files.h before paths.h
#endif

#ifndef FILTERS_H
#error You must include filters.h before paths.h
#endif

#ifndef PARSERS_H
#error You must include parsers.h before paths.h
#endif

#ifndef DB_H
#error You must include db.h before paths.h
#endif

namespace hbackup {

class Path {
  string            _path;
  int               _expiration;
  Parsers           _parsers;
  Filters           _filters;
  SortedList<File>  _list;
  int               _mount_path_length;
  int iterate_directory(
    const string&   path,
    Parser*         parser);
public:
  Path(const string& path);
  string path() { return _path; }
  int expiration() { return _expiration; }
  void setExpiration(int expiration) { _expiration = expiration; }
  list<File>* list() {
    return &_list;
  }
  // Set append to true to add as condition to last added filter
  int addFilter(
    const string& type,
    const string& string,
    bool          append = false);
  int addParser(
    const string& type,
    const string& string);
  int createList(const string& backup_path);
  void clearList() {
    _list.clear();
  }
  // Information
  void showParsers() {
    _parsers.list();
  }
};

class Path2 {
  char*         _path;
  int           _backup_path_length;
  Directory*    _dir;
  Parsers       _parsers;
  Filters       _filters;
  int           _expiration;
  int           _nodes;
  int recurse(
    Database&   db,
    const char* prefix,
    const char* path,
    Directory*  dir,
    Parser*     parser);
public:
  Path2(const char* path);
  ~Path2() {
    free(_path);
    delete _dir;
  }
  const char* path() const     { return _path;       }
  const Directory* dir() const { return _dir;        }
  int expiration() const       { return _expiration; }
  int nodes() const            { return _nodes;      }
  void setExpiration(int expiration) { _expiration = expiration; }
  // Set append to true to add as condition to last added filter
  int addFilter(
    const string& type,
    const string& string,
    bool          append = false);
  int addParser(
    const string& type,
    const string& string);
  // Need prefix to tell database
  int parse(
    Database&   db,
    const char* prefix,
    const char* backup_path);
  // Information
  void showParsers() {
    _parsers.list();
  }
};

}

#endif
