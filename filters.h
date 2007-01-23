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

#ifndef FILTERS_H
#define FILTERS_H

#include "list.h"
#include "filelist.h"

/* The filter stores a list of rules, each containing a list of filters.
 * A rule matches if all filters in it match (AND)
 *    rule = filter AND filter AND ... AND filter
 * A match is obtained if any rule matches (OR)
 *    result = rule OR rule OR ... OR rule
 * Zero is returned when a match was found, non-zero otherwise (a la strcmp).
 */

/* Filter types */
typedef enum {
  filter_path_end,    /* End of file name */
  filter_path_start,  /* Start of file name */
  filter_path_regexp, /* Regular expression on file name */
  filter_size_above,  /* Minimum size (only applies to regular files) */
  filter_size_below   /* Maximum size (only applies to regular files) */
} filter_type_t;

/* Create rules list */
extern int filters_new(list_t **handle);

/* Destroy rules list and all filters in rules */
extern void filters_free(list_t *handle);

/* Create new rule (filters list) */
extern list_t *filters_rule_new(list_t *handle);

/* Add filter to rule */
extern int filters_rule_add(list_t *rule_handle, mode_t file_type,
  filter_type_t type, ...);

/* Check whether any rule matches (i.e. all its filters match) */
extern int filters_match(const list_t *handle, const filedata_t *filedata);

#endif
