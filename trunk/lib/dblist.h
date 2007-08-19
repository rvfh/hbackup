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

#ifndef DBLIST_H
#define DBLIST_H

#include "dbdata.h"

namespace hbackup {

template<class T>
class SortedList : public list<T> {
  typename list<T>::iterator _hint;
public:
  SortedList() {
    _hint = this->end();
  }
  typename list<T>::iterator find(const T& element) {
    if (this->empty()) {
      _hint = this->end();
      return _hint;
    } else {
      // List is ordered, so search from hint to hopefully save time
      typename list<T>::iterator i = _hint;
      if (i == this->end()) {
        i--;
      }
      /* Which direction to go? */
      if (*i < element) {
        /* List is ordered and hint is before the searched pattern: forward */
        while ((i != this->end()) && (*i < element)) {
          i++;
        }
      } else {
        /* List is ordered and hint is after the search pattern: backward */
        while ((i != this->begin()) && (element < *i)) {
          i--;
        }
        /* We must always give the next when the record was not found */
        if (*i < element) {
          i++;
        }
      }
      _hint = i;
      return i;
    }
  }
  typename list<T>::iterator add(const T& element) {
    return insert(find(element), element);
  }
  typename list<T>::iterator erase(typename list<T>::iterator i) {
    if (i == _hint) {
      if (this->size() == 1) {
        _hint = this->end();
      } else
      if (_hint == this->begin()) {
        _hint++;
      } else {
        _hint--;
      }
    }
    return list<T>::erase(i);
  }
  void clear() {
    _hint = this->end();
    list<T>::clear();
  }
  void sort() {
    _hint = this->end();
    list<T>::sort();
  }
  void unique() {
    _hint = this->end();
    list<T>::unique();
  }
};

class DbList : public SortedList<DbData> {
  bool _open;
  int  load_v1(
    FILE* readfile,
    unsigned int offset = 0);
  int  load_v2(
    FILE* readfile,
    unsigned int offset = 0);
public:
  DbList() : _open(false) {}
  bool isOpen() { return _open; }
  // Load list, skipping offset elements
  int  load(
    const string& path,
    const string& filename,
    unsigned int  offset = 0);
  // Save list, creating a backup of the old one first
  int  save(
    const string& path,
    const string& filename,
    bool          backup = false);
  int  save_v2(
    const string& path,
    const string& filename,
    bool          backup = false);
  int  open(
    const string& path,
    const string& filename);
  int  close(
    const string& path,
    const string& filename);
};

}

#endif
