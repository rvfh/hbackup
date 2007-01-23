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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "list.h"

static list_entry_t *list_find_hidden(const list_t *list,
    const char *search_string, list_payload_get_f payload_get) {
  list_entry_t *entry = NULL;

  /* Search the list  (hint is only null if the list is empty) */
  if (list->size != 0) {
    /* Which direction to go? */
    if (list->payload_get == NULL) {
      int breakme = 0;

      /* List is not ordered: search it all */
      while (((entry = list_next(list, entry)) != NULL) && ! breakme) {
        char *string = payload_get(entry->payload);

        if (! strcmp(string, search_string)) {
          breakme = 1;
        }
        free(string);
      }
    } else {
      char *string = NULL;

      /* List is ordered: search from hint to hopefully save time */
      entry = list->hint;
      string = payload_get(entry->payload);
      if (strcmp(string, search_string) < 0) {
        /* List is ordered and hint is before the searched pattern: forward */
        while (strcmp(string, search_string) < 0) {
          entry = entry->next;
          if (entry == NULL) {
            break;
          }
          free(string);
          string = payload_get(entry->payload);
        }
      } else {
        /* List is ordered and hint is after the search pattern: backward */
        while (strcmp(string, search_string) > 0) {
          entry = entry->previous;
          if (entry == NULL) {
            break;
          }
          free(string);
          string = payload_get(entry->payload);
        }
        /* We must always give the next when the record was not found */
        if (entry == NULL) {
          entry = list->first;
        } else if (strcmp(string, search_string)) {
          entry = entry->next;
        }
      }
      free(string);
    }
  }
  return entry;
}


list_entry_t *list_entry_new(void) {
  return (list_entry_t *) (malloc(sizeof(list_entry_t)));
}

void *list_entry_payload(const list_entry_t *entry) {
  return entry->payload;
}


list_t *list_new(list_payload_get_f payload_get) {
  list_t *list = (list_t *) (malloc(sizeof(list_t)));

  if (list != NULL) {
    list->size = 0;
    list->payload_get = payload_get;
    list->first = list->last = list->hint = NULL;
  }
  return list;
}

void list_free(list_t *list) {
  if (list != NULL) {
    if (list->size != 0) {
      list_entry_t *entry = list->first;

      while (entry != NULL) {
        list_entry_t *dump = entry;

        entry = entry->next;
        free(dump->payload);
        free(dump);
      }
    }
    free(list);
  }
}

list_entry_t *list_append(list_t *list, void *payload) {
  list_entry_t *entry = list_entry_new();

  if ((list == NULL) || (entry == NULL)) {
    fprintf(stderr, "list: append: failed\n");
    return NULL;
  }
  entry->payload = payload;
  entry->previous = list->last;
  entry->next = NULL;
  list->last = entry;
  if (entry->previous == NULL) {
    list->first = entry;
  } else {
    entry->previous->next = entry;
  }
  list->hint = entry;
  list->size++;
  return entry;
}

list_entry_t *list_add(list_t *list, void *payload) {
  list_entry_t *entry = list_entry_new();

  if ((list == NULL) || (entry == NULL)) {
    fprintf(stderr, "list: add: failed\n");
    return NULL;
  }
  entry->payload = payload;
  if (list->payload_get == NULL) {
    entry->next = NULL;
  } else {
    char *string = NULL;

    string = list->payload_get(entry->payload);
    entry->next = list_find_hidden(list, string, list->payload_get);
    free(string);
  }
  if (entry->next == NULL) {
    entry->previous = list->last;
    list->last = entry;
  } else {
    entry->previous = entry->next->previous;
    entry->next->previous = entry;
  }
  if (entry->previous == NULL) {
    list->first = entry;
  } else {
    entry->previous->next = entry;
  }
  list->hint = entry;
  list->size++;
  return entry;
}

void *list_remove(list_t *list, list_entry_t *entry) {
  void *payload = entry->payload;

  /* The hint must not be null if the list is not empty */
  if (entry->previous != NULL) {
    /* our previous' next becomes our next */
    entry->previous->next = entry->next;
    list->hint = entry->previous;
  } else {
    /* removing the first record: point first to our next */
    list->hint = list->first = entry->next;
  }
  if (entry->next == NULL) {
    list->last = entry->previous;
  } else {
    entry->next->previous = entry->previous;
  }
  free(entry);
  list->size--;
  return payload;
}

void list_drop(list_t *list_handle, list_entry_t *entry_handle) {
  free(list_remove(list_handle, entry_handle));
}

int list_size(const list_t *list_handle) {
  if (list_handle != NULL) {
    return list_handle->size;
  }
  return -1;
}

list_entry_t *list_previous(const list_t *list, const list_entry_t *entry) {
  if (list == NULL) {
    return NULL;
  } else if (entry == NULL) {
    return list->last;
  } else {
    return entry->previous;
  }
}

list_entry_t *list_next(const list_t *list, const list_entry_t *entry) {
  if (list == NULL) {
    return NULL;
  } else if (entry == NULL) {
    return list->first;
  } else {
    return entry->next;
  }
}

int list_find(const list_t *list, const char *search_string,
    list_payload_get_f payload_get, list_entry_t **entry_handle) {
  list_entry_t    *entry;
  char            *string = NULL;
  int             result;

  /* Impossible to search without interpreting the payload */
  if (payload_get == NULL) {
    /* If no function was given, use the default one, or exit */
    if (list->payload_get == NULL) {
      fprintf(stderr, "list: find: no way to interpret payload\n");
      return 2;
    }
    payload_get = list->payload_get;
  }

  entry = list_find_hidden(list, search_string, payload_get);
  if (entry_handle != NULL) {
    *entry_handle = entry;
  }
  if (entry == NULL) {
    return 1;
  }
  string = payload_get(entry->payload);
  result = strcmp(string, search_string);
  free(string);

  return result;
}

void list_show(const list_t *list, list_entry_t *entry_handle,
    list_payload_get_f payload_get) {
  static int level = 0;

  if (list != NULL) {
    if (payload_get == NULL) {

      /* If no function was given, use the default one, or exit */
      if (list->payload_get == NULL) {
        fprintf(stderr, "list: show: no way to interpret payload\n");
        return;
      }
      payload_get = list->payload_get;
    }

    if (entry_handle != NULL) {
      if (list_entry_payload(entry_handle) != NULL) {
        char *string = payload_get(list_entry_payload(entry_handle));
        if (string != NULL) {
          int i;

          for (i = 0; i < level; i++) {
            printf("-");
          }
          printf("> %s\n", string);
          free(string);
        }
      } else {
        fprintf(stderr, "list: show: no payload!\n");
      }
    } else {
      level++;
      while ((entry_handle = list_next(list, entry_handle)) != NULL) {
        list_show(list, entry_handle, payload_get);
      }
      level--;
    }
  }
}

int list_compare(
    const list_t            *list_left,
    const list_t            *list_right,
    list_t                  **list_added_handle,
    list_t                  **list_missing_handle,
    list_payloads_compare_f compare_f) {
  list_t        *list_added   = NULL;
  list_t        *list_missing = NULL;
  /* Point the entries to the start of their respective lists */
  list_entry_t  *entry_left   = list_next(list_left, NULL);
  list_entry_t  *entry_right  = list_next(list_right, NULL);
  /* Comparison result */
  int           differ        = 0;

  /* Impossible to compare without interpreting the payloads */
  if (list_left->payload_get == NULL) {
    fprintf(stderr, "list: compare: no way to interpret left payload\n");
    return 2;
  }
  if (list_right->payload_get == NULL) {
    fprintf(stderr, "list: compare: no way to interpret right payload\n");
    return 2;
  }

  if (list_added_handle != NULL) {
    if ((list_added = list_new(list_right->payload_get)) == NULL) {
      fprintf(stderr, "list: compare: cannot create added list\n");
      return 2;
    }
    *list_added_handle = list_added;
  }

  if (list_missing_handle != NULL) {
    if ((list_missing = list_new(list_left->payload_get)) == NULL) {
      fprintf(stderr, "list: compare: cannot create missing list\n");
      return 2;
    }
    *list_missing_handle = list_missing;
  }

  while ((entry_left != NULL) || (entry_right != NULL)) {
    int result;
    void *payload_left  = NULL;
    void *payload_right = NULL;

    if (entry_left != NULL) {
      payload_left = list_entry_payload(entry_left);
    }
    if (entry_right != NULL) {
      payload_right = list_entry_payload(entry_right);
    }

    if (payload_left == NULL) {
      result = 1;
    } else if (payload_right == NULL) {
      result = -1;
    } else if (compare_f != NULL) {
      result = compare_f(payload_left, payload_right);
    } else {
      char  *string_left  = list_left->payload_get(payload_left);
      char  *string_right = list_right->payload_get(payload_right);

      result = strcmp(string_left, string_right);
      free(string_left);
      free(string_right);
    }
    differ |= (result != 0);
    /* left < right => element is missing from right list */
    if ((result < 0) && (list_missing != NULL)) {
      /* The contents are NOT copied, so the two lists have elements
       * pointing to the same data! */
      list_append(list_missing, payload_left);
    }
    /* left > right => element was added in right list */
    if ((result > 0) && (list_added != NULL)) {
      /* The contents are NOT copied, so the two lists have elements
       * pointing to the same data! */
      list_append(list_added, payload_right);
    }
    if (result >= 0) {
      entry_right = list_next(list_right, entry_right);
    }
    if (result <= 0) {
      entry_left = list_next(list_left, entry_left);
    }
  }
  if (differ) {
    return 1;
  }
  return 0;
}

int list_select(
    const list_t *list,
    const char *search_string,
    list_payload_get_f payload_get,
    list_t **list_selected_handle,
    list_t **list_unselected_handle) {
  list_entry_t  *entry = NULL;
  int             length = strlen(search_string);

  /* Note: list_find does not garantee to find the first element */
  if (list_selected_handle != NULL) {
    *list_selected_handle   = NULL;
  }
  if (list_unselected_handle != NULL) {
    *list_unselected_handle = NULL;
  }

  /* Impossible to search without interpreting the payload */
  if (payload_get == NULL) {
    /* If no function was given, use the default one, or exit */
    if (list->payload_get == NULL) {
      fprintf(stderr, "list: select: no way to interpret payload\n");
      return 2;
    }
    payload_get = list->payload_get;
  }

  /* Search the complete list, but stop when no more match is found */
  if (list_selected_handle != NULL) {
    *list_selected_handle   = list_new(list->payload_get);
  }
  if (list_unselected_handle != NULL) {
    *list_unselected_handle = list_new(list->payload_get);
  }
  while ((entry = list_next(list, entry)) != NULL) {
    void *payload = list_entry_payload(entry);

    if (payload != NULL) {
      char *string = NULL;
      int  result;

      string = payload_get(payload);
      /* Only compare the portion of string of search_string's length */
      result = strncmp(string, search_string, length);
      if (result == 0) {
        /* Portions of strings match */
        if (list_selected_handle != NULL) {
          list_append(*list_selected_handle, payload);
        }
      } else if (list_unselected_handle != NULL) {
        /* Portions of strings do not match */
        list_append(*list_unselected_handle, payload);
      } else if ((result == 1) && (payload_get == list->payload_get)) {
        /* When using the default payload function, time can be saved */
        break;
      }
      free(string);
    } else {
      fprintf(stderr, "list: select: found null payload, ignoring\n");
    }
  }
  return 0;
}

int list_deselect(list_t *list) {
  list_entry_t *entry = NULL;

  while ((entry = list_next(list, entry)) != NULL) {
    free(entry);
    list->size--;
  }
  /* Free list only if empty */
  if (list->size == 0) {
    free(list);
  }
  return list->size;
}
