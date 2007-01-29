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

#include <vector>

#ifndef COMMON_H
#error You must include common.h before filters.h
#endif

using namespace std;

/* The filter stores a list of rules, each containing a list of conditions.
 * A rule matches if all conditions in it match (AND)
 *    rule = condition AND condition AND ... AND condition
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

class Condition {
  filter_type_t _type;
  mode_t        _file_type;
  off_t         _size;
  char          _string[256];
public:
  Condition(
    mode_t file_type,
    filter_type_t type,
    ...);
  int  match(const filedata_t *filedata) const;
  /* Debug only */
  void show() const;
};

class Rule: public vector<Condition*> {
public:
  Rule() {}
  Rule(Condition *condition);
  ~Rule();
};

class Filter: public vector<Rule*> {
public:
  ~Filter();
  int match(const filedata_t *filedata) const;
};

#endif
