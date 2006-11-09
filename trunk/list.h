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

typedef void *list_t;
typedef void *list_entry_t;

/* Functions */

/* Create empty list entry, return it */
extern list_entry_t list_entry_new(void);

/* Get list entry payload */
extern void *list_entry_payload(const list_entry_t entry_handle);

/* Create empty list, return it */
extern list_t list_new(list_payload_get_f payload_get_f);

/* Destroy list and its contents */
extern void list_free(list_t list_handle);

/* Append given entry to list */
extern list_entry_t list_append(list_t list_handle, void *payload);

/* Add new list entry with given payload, return pointer to entry or NULL */
extern list_entry_t list_add(list_t list_handle, void *payload);

/* Remove entry from list and destroy it, return its contents */
extern void *list_remove(list_t list_handle, list_entry_t entry_handle);

/* Remove entry from list, destroy it and its contents */
extern void list_drop(list_t list_handle, list_entry_t entry_handle);

/* Get list size */
extern int list_size(const list_t list_handle);

/* Get previous entry */
extern list_entry_t list_previous(const list_t list_handle,
  const list_entry_t entry_handle);

/* Get next entry */
extern list_entry_t list_next(const list_t list_handle,
  const list_entry_t entry_handle);

/* Find record in ordered list, or its next neighbour, by matching
 * search_string with the result of payload_get_f
 * If payload_get_f is NULL, the list default is used
 * If entry_handle is not NULL, it will contain the closest entry found
 * If an exact match is found, the function returns 0, non zero otherwise */
extern int list_find(const list_t list_handle, const char *search_string,
  list_payload_get_f payload_get_f, list_entry_t *entry_handle);

/* Show list contents (payload contents can be included) */
/* If payload_get_f is NULL, the list default is used */
extern void list_show(const list_t list_handle, list_entry_t *entry,
  list_payload_get_f payload_get_f);

/* Compare two lists, using compare function
 * if compare_f is NULL, the default payload functions and strcmp are used
 * If list_added_handle is not NULL, it will contain the list of added entries
 * If list_missing_handle is not NULL, it will contain the list of removed e.
 * Important note: the lists elements payloads are not copied!!!
 * To delete the added and removed elements lists, call list_deselect or do a
 * list_remove on each element before calling list_free. */
extern int list_compare(
    const list_t            list_left_handle,
    const list_t            list_right_handle,
    list_t                  *list_added_handle,
    list_t                  *list_missing_handle,
    list_payloads_compare_f compare_f);

/* Select records in list, by matching search_string with the result of
 * payload_get_f. If payload_get_f is NULL, the list default is used
 * On success the function returns 0, otherwise non zero and list will be NULL.
 * Important note: the list elements payloads are not copied!!!
 * To delete the returned list, call list_deselect or do a list_remove on each
 * element before calling list_free. */
extern int list_select(const list_t list_handle, const char *search_string,
  list_payload_get_f payload_get_f, list_t *list_selected_handle);

/* Free list and its elements without freeing the payloads
 * Returns the number of elements not freed */
extern int list_deselect(list_t list_handle);

#endif
