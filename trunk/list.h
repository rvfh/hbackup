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

/* Type for function to convert payload into usable data
 * *string_p must be allocated by the function, but is freed by this module
 */
typedef char *(list_payload_get_f) (const void *payload);

/* Type for function to compare payloads from two lists
 * Return codes:
 * <0 left data is less than right data / add left to missing list
 * =0 left data is same as right data / skip both
 * >0 left data is more than right data / add right to added list
 * The pointed is not const, so the function can actually alter it if needed.
 */
typedef int (list_payloads_compare_f) (void *payload_left,
  void *payload_right);

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
    const char              *search_string,
    list_payload_get_f      payload_get = NULL);
public:
  List(list_payload_get_f payload_get_f = NULL);
  ~List();
  bool ordered() const;
  char *payloadString(const void *payload) const;
  list_entry_t *append(void *payload);
  list_entry_t *add(void *payload);
  void *remove(list_entry_t *entry);
  void drop(list_entry_t *entry);
  int size() const;
  list_entry_t *first() const { return _first; }
  list_entry_t *previous(const list_entry_t *entry) const;
  list_entry_t *next(const list_entry_t *entry) const;
  int find(const char *search_string, list_payload_get_f payload_get_f, list_entry_t **entry_handle);
  void show(
    list_entry_t            *entry                = NULL,
    list_payload_get_f      payload_get_f         = NULL) const;
  int compare(
    const List              *other_list,
    List                    *list_added_handle    = NULL,
    List                    *list_missing_handle  = NULL,
    list_payloads_compare_f compare_f             = NULL) const;
  int select(
    const char              *search_string,
    list_payload_get_f      payload_get_f,
    List                    *list_selected_handle,
    List                    *list_unselected_handle) const;
  int deselect();
};

#endif
