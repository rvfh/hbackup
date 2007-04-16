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

#include <iostream>
#include <vector>
#include <stdarg.h>
#include <regex.h>
#include <sys/stat.h>

using namespace std;

#include "files.h"
#include "filters.h"

int Condition::match(const File& filedata) const {
  switch(_type) {
  case filter_type:
    if ((_file_type & filedata.type()) == 0) {
      return 1;
    }
  case filter_path_end: {
    signed int diff = filedata.path().size() - _string.size();
    if (diff < 0) {
      return 1;
    }
    return _string != filedata.path().substr(diff); }
  case filter_path_start:
    return filedata.path().substr(0, _string.size()) != _string;
  case filter_path_regexp:
    regex_t regex;
    if (regcomp(&regex, _string.c_str(), REG_EXTENDED)) {
      cerr << "filters: regexp: incorrect expression" << endl;
      return 2;
    }
    return regexec(&regex, filedata.path().c_str(), 0, NULL, 0);
  case filter_size_above:
    return filedata.size() < _size;
  case filter_size_below:
    return filedata.size() > _size;
  default:
    cerr << "filters: match: unknown condition type" << endl;
    return 2;
  }
  return 1;
}

void Condition::show() const {
  switch (_type) {
    case filter_path_end:
    case filter_path_start:
    case filter_path_regexp:
      cout << "--> " << _string << " " << _type << endl;
      break;
    case filter_size_above:
    case filter_size_below:
      cout << "--> " << _size << " " << _type << endl;
      break;
    default:
      cout << "--> unknown condition type " << _type << endl;
  }
}

int Filters::match(const File& filedata) const {
  /* Read through list of rules */
  for (unsigned int i = 0; i < size(); i++) {
    Filter  rule  = (*this)[i];
    int     match = 1;

    /* Read through list of conditions in rule */
    for (unsigned int j = 0; j < rule.size(); j++) {
      Condition condition = rule[j];

      /* All filters must match for rule to match */
      if (condition.match(filedata)) {
        match = 0;
        break;
      }
    }
    /* If all conditions matched, or the rule is empty, we have a rule match */
    if (match) {
      return 0;
    }
  }
  /* No match */
  return 1;
}
