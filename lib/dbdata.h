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
public:
  DbData(const DbData& data) :
      _prefix(NULL),
      _path(NULL),
      _node(NULL) {
    asprintf(&_prefix, "%s", data._prefix);
    asprintf(&_path, "%s", data._path);
    switch (data._node->type()) {
      case 'l':
        _node = new Link(*((Link*)data._node));
        break;
      case 'f':
        _node = new File(*((File*)data._node));
        break;
      default:
        _node = new Node(*data._node);
    }
  }
  DbData(
      const char* prefix,
      const char* path,
      Node*       data) :
      _prefix(NULL),
      _path(NULL),
      _node(data) {
    asprintf(&_prefix, "%s", prefix);
    asprintf(&_path, "%s", path);
  }
  ~DbData() {
    free(_prefix);
    free(_path);
    free(_node);
  }
  const Node* data()            { return _node; }
  const char* prefix() const    { return _prefix; }
  const char* path() const      { return _path; }
  int pathCompare(const char* path, int length = -1) {
    char* full_path = NULL;
    asprintf(&full_path, "%s/%s", _prefix, _path);
    int cmp = Node::pathCompare(full_path, path, length);
    free(full_path);
    return cmp;
  }
  void line() {
    printf("%s\t%s\t%c\t%lld\t%d\t%u\t%u\t%o",
      _prefix, _path, _node->type(), _node->size(), _node->mtime() != 0,
      _node->uid(), _node->gid(), _node->mode());
    if (_node->type() == 'l') {
      printf("\t");
      printf(((Link*)_node)->link());
    }
    if (_node->type() == 'f') {
      printf("\t");
      printf(((File*)_node)->checksum());
    }
    printf("\n");
  }
};

}

#endif
