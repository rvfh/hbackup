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
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <regex.h>
#include "list.h"
#include "metadata.h"
#include "common.h"
#include "filters.h"

Condition::Condition(
    mode_t file_type,
    filter_type_t type,
    ...) {
  va_list  args;

  _type = type;
  va_start(args, type);
  switch (type) {
    case filter_path_end:
    case filter_path_start:
    case filter_path_regexp:
      strncpy(_string, va_arg(args, char *), 255);
      _file_type  = file_type;
      break;
    case filter_size_above:
    case filter_size_below:
      _size       = va_arg(args, size_t);
      _file_type  = S_IFREG;
      break;
    default:
      fprintf(stderr, "filters: add: unknown filter type\n");
  }
  va_end(args);
}

int Condition::match(const filedata_t *filedata) const {
  /* Check that file type matches */
  if ((_file_type & filedata->metadata.type) == 0) {
    return 1;
  }
  /* Run filters */
  switch(_type) {
  case filter_path_end: {
    signed int diff = strlen(filedata->path) - strlen(_string);
    if (diff < 0) {
      return 1;
    }
    return strcmp(_string, &filedata->path[diff]); }
  case filter_path_start:
    return strncmp(filedata->path, _string, strlen(_string));
  case filter_path_regexp:
    regex_t regex;
    if (regcomp(&regex, _string, REG_EXTENDED)) {
      fprintf(stderr, "filters: regexp: incorrect expression\n");
      return 2;
    }
    return regexec(&regex, filedata->path, 0, NULL, 0);
  case filter_size_above:
    return filedata->metadata.size < _size;
  case filter_size_below:
    return filedata->metadata.size > _size;
  default:
    fprintf(stderr, "filters: match: unknown filter type\n");
    return 2;
  }
  return 1;
}

char *Condition::show() const {
  char  *string = NULL;

  switch (_type) {
    case filter_path_end:
    case filter_path_start:
    case filter_path_regexp:
      asprintf(&string, "%s %u", _string, _type);
      break;
    case filter_size_above:
    case filter_size_below:
      asprintf(&string, "%ld %u", _size, _type);
      break;
    default:
      asprintf(&string, "unknown filter type");
  }
  return string;
}

Rule::Rule(Condition *condition) {
  if (condition != NULL) {
    addCondition(condition);
  }
}

Rule::~Rule() {
  /* Note: entry gets free in the loop */
  list_entry_t *condition;

  /* List of lists, free each embedded list */
  while ((condition = next(NULL)) != NULL) {
    delete (Condition *) remove(condition);
  }
}

int Rule::addCondition(Condition *condition) {
  if (this->append(condition) == NULL) {
    fprintf(stderr, "filters: addCondition: failed\n");
    return -1;
  }
  return 0;
}

Filter::~Filter() {
  /* Note: entry gets free in the loop */
  list_entry_t *rule;

  /* List of lists, free each embedded list */
  while ((rule = next(NULL)) != NULL) {
    delete (Rule *) remove(rule);
  }
}

int Filter::addRule(Rule *rule) {
  if (this->append(rule) == NULL) {
    fprintf(stderr, "filters: addRule: failed\n");
    return -1;
  }
  return 0;
}

int Filter::match(const filedata_t *filedata) const {
  list_entry_t *rule_entry = NULL;

  /* Read through list of rules */
  while ((rule_entry = this->next(rule_entry)) != NULL) {
    Rule          *rule         = (Rule *) (list_entry_payload(rule_entry));
    list_entry_t  *filter_entry = NULL;
    int           match         = 1;

    /* Read through list of filters in rule */
    while ((filter_entry = rule->next(filter_entry)) != NULL) {
      Condition *filter = (Condition *) (list_entry_payload(filter_entry));

      /* All filters must match for rule to match */
      if (filter->match(filedata)) {
        match = 0;
        break;
      }
    }
    /* If all filters matched (or the rule is empty!), we have a rule match */
    if (match) {
      return 0;
    }
  }
  /* No match */
  return 1;
}
