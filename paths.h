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

#ifndef PATHS_H
#define PATHS_H

#ifndef LIST_H
#error You must include list.h before paths.h
#endif

#ifndef FILTERS_H
#error You must include filters.h before paths.h
#endif

#ifndef PARSERS_H
#error You must include parsers.h before paths.h
#endif

class Path {
  string  _path;
  Parsers _parsers;
  Filters _filters;
  List*   _list;
  int     _mount_path_length;
  int iterate_directory(
    const string&   path,
    Parser*         parser);
public:
  Path(const string& path);
  ~Path() { delete _list; }
  string path() {
    return _path;
  }
  List* list() {
    return _list;
  }
  int addFilter(
    const string& type,
    const string& string);
  int addParser(
    const string& type,
    const string& string);
  int backup(const string& backup_path);
};

#endif
