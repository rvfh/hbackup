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
  int select(
    const string&           search_string,
    list_payload_get_f      payload_get_f,
    List                    *list_selected_handle,
    List                    *list_unselected_handle) const;
  int deselect();
};

#endif
