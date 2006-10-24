/* Herve Fache

20061003 Creation
*/

#ifndef LIST_H
#define LIST_H

/* Type for function to convert payload into usable data
 * string can be at most FILENAME_MAX long
 */
typedef void (list_payload_get_f) (const void *payload, char *string);

/* Type for function to compare payloads from two lists
 * Return codes:
 * -3 left data out of bounds (no need to check anymore)
 * -2 left data should be ignored
 * -1 left data is less than right data
 *  0 left data is same as right data
 *  1 left data is more than right data
 *  2 right data should be ignored
 *  3 right data out of bounds (no need to check anymore)
 * The pointed is not const, so the function can actually alter it if needed.
 */
typedef int (list_payloads_compare_f) (void *payload_left, void *payload_right);

typedef void *list_t;
typedef void *list_entry_t;

/* Functions */

/* Create empty list entry */
extern list_entry_t list_entry_new(void);

/* Get list entry payload */
extern void *list_entry_payload(const list_entry_t entry_handle);

/* Create empty list */
extern list_t list_new(list_payload_get_f payload_get_f);

/* Destroy list and its contents */
extern void list_free(list_t list_handle);

/* Append given entry to list */
extern int list_append(list_t list_handle, void *payload);

/* Add given entry to list */
extern int list_add(list_t list_handle, void *payload);

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

/* Find record in ordered list, or its next neighbour */
/* If payload_get_f is NULL, the list default is used */
/* If entry_handle is not NULL, is will contain the closest entry found */
/* If an exact match is found, the function returns 0, non zero otherwise */
extern int list_find(const list_t list_handle, const char *search_string,
  list_payload_get_f payload_get_f, list_entry_t **entry_handle);

/* Show list contents (payload contents can be included) */
/* If payload_get_f is NULL, the list default is used */
extern void list_show(const list_t list_handle, list_entry_t *entry,
  list_payload_get_f payload_get_f);

extern int list_compare(
    const list_t            list_left_handle,
    const list_t            list_right_handle,
    list_t                  *list_added_handle,
    list_t                  *list_missing_handle,
    list_payloads_compare_f compare_f);

#endif
