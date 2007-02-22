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

#ifndef LIST_H
#define LIST_H

template<class T>
class SortedList : public list<T> {
  typename list<T>::iterator _hint;
public:
  typename list<T>::iterator find(const T& element) {
    if (this->empty()) {
      _hint = this->end();
      return _hint;
    } else {
      // List is ordered, so search from hint to hopefully save time
      list<string>::iterator i = _hint;
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
  void add(const T& element) {
    list<string>::iterator i = find(element);
    if (i == this->end()) {
      push_back(T(element));
    } else {
      insert(i, element);
    }
  }
  list<string>::iterator erase(list<string>::iterator i) {
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
};

// Type for function to convert payload into usable data
typedef string (list_payload_get_f) (const void *payload);

typedef struct _list_entry_t list_entry_t;

struct _list_entry_t {
  void                *payload;
  list_entry_t        *previous;
  list_entry_t        *next;
};

/* Functions */

/* Create empty list entry, return it */
extern list_entry_t *list_entry_new();

/* Get list entry payload */
extern void *list_entry_payload(const list_entry_t *entry);

/* Classes */

class List {
  list_entry_t        *_first;
  list_entry_t        *_last;
  list_entry_t        *_hint;
  list_payload_get_f  *_payload_get_f;
  int                 _size;
  list_entry_t        *find_hidden(
    const string&           search_string,
    list_payload_get_f      payload_get = NULL) const;
public:
  List(list_payload_get_f payload_get_f);
  ~List();
  string payloadString(const void *payload) const;
  list_entry_t *append(void *payload);
  list_entry_t *add(void *payload);
  void *remove(list_entry_t *entry);
  int size() const;
  list_entry_t *next(const list_entry_t *entry) const;
  int find(
    const string&           search_string,
    list_payload_get_f      payload_get_f,
    list_entry_t**          entry_handle) const;
  void show(
    list_entry_t            *entry                = NULL,
    list_payload_get_f      payload_get_f         = NULL) const;
};

#endif
