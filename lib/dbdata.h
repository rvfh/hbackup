/*
     Copyright (C) 2007  Herve Fache

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

#ifndef DBDATA_H
#define DBDATA_H

#include "files.h"

namespace hbackup {

class DbData {
  char*   _prefix;
  char*   _path;
  Node*   _node;
  time_t  _in;
  time_t  _out;
public:
  DbData(const DbData& data) :
      _prefix(NULL),
      _path(NULL),
      _node(NULL),
      _in(data._in),
      _out(data._out) {
    asprintf(&_prefix, "%s", data._prefix);
    asprintf(&_path, "%s", data._path);
    switch (data._node->type()) {
      case 'l':
        _node = new Link(*((Link*)data._node));
        break;
      case 'f':
        _node = new File2(*((File2*)data._node));
        break;
      default:
        _node = new Node(*data._node);
    }
  }
  DbData(
      const char* prefix,
      const char* path,
      Node*       data,
      time_t      in = 0) :
      _prefix(NULL),
      _path(NULL),
      _node(data),
      _out(0) {
    asprintf(&_prefix, "%s", prefix);
    asprintf(&_path, "%s", path);
    if (in == 0) {
      _in = time(NULL);
    } else {
      _in = in;
    }
  }
  ~DbData() {
    free(_prefix);
    free(_path);
    free(_node);
  }
  bool operator<(const DbData& right) const {
    int cmp = strcmp(_prefix, right._prefix);
    if (cmp < 0)      return true;
    else if (cmp > 0) return false;

    // This will hopefully go when we just follow the Directory order
    string path1 = _path;
    string path2 = right._path;
    unsigned int pos = 0;
    while ((pos = path1.find('/', pos)) != string::npos) {
      path1.replace(pos, 1, "\31");
    }
    pos = 0;
    while ((pos = path2.find('/', pos)) != string::npos) {
      path2.replace(pos, 1, "\31");
    }
    cmp = path1.compare(path2);
    if (cmp < 0)      return true;
    else if (cmp > 0) return false;

    // Equal then...
    return _in < right._in;
  }
  const Node* data()            { return _node; }
  const char* prefix() const    { return _prefix; }
  const char* path() const      { return _path; }
  time_t      in() const        { return _in; }
  time_t      out() const        { return _out; }
  void setOut() { _out = time(NULL); }
  void setOut(time_t out) { _out = out; }
  int comparePath(const char* path, int length = -1) {
    char* full_path = NULL;
    asprintf(&full_path, "%s/%s", _prefix, _path);
    int cmp;
    if (length >= 0) {
      cmp = strncmp(full_path, path, length);
    } else {
      cmp = strcmp(full_path, path);
    }
    free(full_path);
    return cmp;
  }
  void line() {
    printf("%s\t%s\t%c\t%lld\t%d\t%u\t%u\t%o\t",
      _prefix, _path, _node->type(), _node->size(), _node->mtime() != 0,
      _node->uid(), _node->gid(), _node->mode());
    if (_node->type() == 'l') {
      printf(((Link*)_node)->link());
    }
    printf("\t");
    if (_node->type() == 'f') {
      printf(((File2*)_node)->checksum());
    }
    printf("\t%ld\t%ld\t\n", _in, _out);
  }
};

}

#endif
