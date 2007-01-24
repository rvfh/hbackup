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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <iostream>
using namespace std;
#include "filters.cpp"

static char *filters_show(const void *payload) {
  const Condition *condition = (const Condition *) (payload);

  return condition->show();
}

static char *filters_rule_show(const void *payload) {
  const List  *rule   = (const List *) payload;
  char        *string = NULL;

  cout << "-> List " << rule->size() << " filter(s)\n";
  rule->show(NULL, filters_show);
  return string;
}

/* TODO Test file type check */
int main(void) {
  Filter      *filter = NULL;
  Filter      *filter2 = NULL;
  Rule        *rule;
  Condition   *condition;
  filedata_t  filedata;

  filedata.metadata.type = S_IFREG;

  cout << "Filters test\n";
  cout << "filter_path_end_check\n";
  condition = new Condition(S_IFREG, filter_path_end, ".txt");
  filedata.path = "to a file.txt";
  if (! condition->match(&filedata)) {
    cout << "match 1.1\n";
  }
  filedata.path = "to a file.tst";
  if (! condition->match(&filedata)) {
    cout << "match 1.2\n";
  }
  delete condition;

  cout << "filter_path_start_check\n";
  condition = new Condition(S_IFREG, filter_path_start, "this is/a");
  filedata.path = "this is/a path/to a file.txt";
  if (! condition->match(&filedata)) {
    cout << "match 2.1\n";
  }
  delete condition;
  condition = new Condition(S_IFREG, filter_path_start, "this was/a");
  if (! condition->match(&filedata)) {
    cout << "match 2.2\n";
  }
  delete condition;

  cout << "filter_path_regexp_check\n";
  condition = new Condition(S_IFREG, filter_path_regexp, "^this.*path/.*\\.txt");
  if (! condition->match(&filedata)) {
    cout << "match 3.1\n";
  }
  delete condition;
  condition = new Condition(S_IFREG, filter_path_regexp, "^this.*path/a.*\\.txt");
  if (! condition->match(&filedata)) {
    cout << "match 3.2\n";
  }
  delete condition;

  /* No filter check possible for size comparison: not a function */

  cout << "\nMatch function test\n";
  condition = new Condition(S_IFREG, filter_size_below, 5000);
  filedata.metadata.size = 4000;
  if (condition->match(&filedata)) {
    cout << "Not matching " << filedata.metadata.size << "\n";
  } else {
    cout << "Matching " << filedata.metadata.size << "\n";
  }
  filedata.metadata.size = 6000;
  if (condition->match(&filedata)) {
    cout << "Not matching " << filedata.metadata.size << "\n";
  } else {
    cout << "Matching " << filedata.metadata.size << "\n";
  }

  cout << "\nSimple rules test\n";
  filter = new Filter;
  if (filter->addRule(new Rule(new Condition(S_IFREG, filter_path_regexp, "^to a.*\\.txt")))) {
    cout << "Failed to add\n";
  } else {
    cout << ">List " << filter->size() << " rule(s):\n";
    filter->show(NULL, filters_rule_show);
  }
  if (filter->addRule(new Rule(new Condition(S_IFREG, filter_path_regexp, "^to a.*\\.t.t")))) {
    cout << "Failed to add\n";
  } else {
    cout << ">List " << filter->size() << " rule(s):\n";
    filter->show(NULL, filters_rule_show);
  }
  filedata.path = "to a file.txt";
  if (filter->match(&filedata)) {
    cout << "Not matching 1\n";
  }
  filedata.path = "to a file.tst";
  if (filter->match(&filedata)) {
    cout << "Not matching 2\n";
  }
  filedata.path = "to a file.tsu";
  if (filter->match(&filedata)) {
    cout << "Not matching 3\n";
  }

  filter2 = new Filter;
  filedata.path = "to a file.txt";
  if (filter2->match(&filedata)) {
    cout << "Not matching +1\n";
  }
  filedata.path = "to a file.tst";
  if (filter2->match(&filedata)) {
    cout << "Not matching +2\n";
  }
  filedata.path = "to a file.tsu";
  if (filter2->match(&filedata)) {
    cout << "Not matching +3\n";
  }
  if (filter2->addRule(new Rule(new Condition(S_IFREG, filter_path_regexp, "^to a.*\\.txt")))) {
    cout << "Failed to add\n";
  } else {
    cout << ">List " << filter2->size() << " rule(s):\n";
    filter2->show(NULL, filters_rule_show);
  }
  if (filter2->addRule(new Rule(new Condition(S_IFREG, filter_path_regexp, "^to a.*\\.t.t")))) {
    cout << "Failed to add\n";
  } else {
    cout << ">List " << filter2->size() << " rule(s):\n";
    filter2->show(NULL, filters_rule_show);
  }
  filedata.path = "to a file.txt";
  if (filter2->match(&filedata)) {
    cout << "Not matching +1\n";
  }
  filedata.path = "to a file.tst";
  if (filter2->match(&filedata)) {
    cout << "Not matching +2\n";
  }
  filedata.path = "to a file.tsu";
  if (filter2->match(&filedata)) {
    cout << "Not matching +3\n";
  }
  delete filter2;

  filedata.path = "to a file.txt";
  if (filter->match(&filedata)) {
    cout << "Not matching 1\n";
  }
  filedata.path = "to a file.tst";
  if (filter->match(&filedata)) {
    cout << "Not matching 2\n";
  }
  filedata.path = "to a file.tsu";
  if (filter->match(&filedata)) {
    cout << "Not matching 3\n";
  }
  delete filter;

  filter = new Filter;
  filedata.metadata.size = 0;
  if (filter->match(&filedata)) {
    cout << "Not matching " << filedata.metadata.size << "\n";
  } else {
    cout << "Matching " << filedata.metadata.size << "\n";
  }
  filedata.metadata.size = 1000;
  if (filter->match(&filedata)) {
    cout << "Not matching " << filedata.metadata.size << "\n";
  } else {
    cout << "Matching " << filedata.metadata.size << "\n";
  }
  filedata.metadata.size = 1000000;
  if (filter->match(&filedata)) {
    cout << "Not matching " << filedata.metadata.size << "\n";
  } else {
    cout << "Matching " << filedata.metadata.size << "\n";
  }
  /* File type is always S_IFREG */
  if (filter->addRule(new Rule(new Condition(0, filter_size_below, 500)))) {
    cout << "Failed to add\n";
  } else {
    cout << ">List " << filter->size() << " rule(s):\n";
    filter->show(NULL, filters_rule_show);
  }
  filedata.metadata.size = 0;
  if (filter->match(&filedata)) {
    cout << "Not matching " << filedata.metadata.size << "\n";
  } else {
    cout << "Matching " << filedata.metadata.size << "\n";
  }
  filedata.metadata.size = 1000;
  if (filter->match(&filedata)) {
    cout << "Not matching " << filedata.metadata.size << "\n";
  } else {
    cout << "Matching " << filedata.metadata.size << "\n";
  }
  filedata.metadata.size = 1000000;
  if (filter->match(&filedata)) {
    cout << "Not matching " << filedata.metadata.size << "\n";
  } else {
    cout << "Matching " << filedata.metadata.size << "\n";
  }
  /* File type is always S_IFREG */
  if (filter->addRule(new Rule(new Condition(0, filter_size_above, 5000)))) {
    cout << "Failed to add\n";
  } else {
    cout << ">List " << filter->size() << " rule(s):\n";
    filter->show(NULL, filters_rule_show);
  }
  filedata.metadata.size = 0;
  if (filter->match(&filedata)) {
    cout << "Not matching " << filedata.metadata.size << "\n";
  } else {
    cout << "Matching " << filedata.metadata.size << "\n";
  }
  filedata.metadata.size = 1000;
  if (filter->match(&filedata)) {
    cout << "Not matching " << filedata.metadata.size << "\n";
  } else {
    cout << "Matching " << filedata.metadata.size << "\n";
  }
  filedata.metadata.size = 1000000;
  if (filter->match(&filedata)) {
    cout << "Not matching " << filedata.metadata.size << "\n";
  } else {
    cout << "Matching " << filedata.metadata.size << "\n";
  }
  delete filter;

  /* Test complex rules */
  cout << "\nComplex rules test\n";
  filter = new Filter;
  rule   = new Rule;

  cout << ">List " << filter->size() << " rule(s):\n";
  filter->show(NULL, filters_rule_show);

  filter->addRule(rule);
  if (rule->addCondition(new Condition(0, filter_size_below, 500))) {
    cout << "Failed to add\n";
  } else {
    cout << ">List " << filter->size() << " rule(s):\n";
    filter->show(NULL, filters_rule_show);
  }
  if (rule->addCondition(new Condition(0, filter_size_above, 400))) {
    cout << "Failed to add\n";
  } else {
    cout << ">List " << filter->size() << " rule(s):\n";
    filter->show(NULL, filters_rule_show);
  }
  filedata.metadata.size = 600;
  if (filter->match(&filedata)) {
    cout << "Not matching " << filedata.metadata.size << "\n";
  } else {
    cout << "Matching " << filedata.metadata.size << "\n";
  }
  filedata.metadata.size = 500;
  if (filter->match(&filedata)) {
    cout << "Not matching " << filedata.metadata.size << "\n";
  } else {
    cout << "Matching " << filedata.metadata.size << "\n";
  }
  filedata.metadata.size = 450;
  if (filter->match(&filedata)) {
    cout << "Not matching " << filedata.metadata.size << "\n";
  } else {
    cout << "Matching " << filedata.metadata.size << "\n";
  }
  filedata.metadata.size = 400;
  if (filter->match(&filedata)) {
    cout << "Not matching " << filedata.metadata.size << "\n";
  } else {
    cout << "Matching " << filedata.metadata.size << "\n";
  }
  filedata.metadata.size = 300;
  if (filter->match(&filedata)) {
    cout << "Not matching " << filedata.metadata.size << "\n";
  } else {
    cout << "Matching " << filedata.metadata.size << "\n";
  }

  delete filter;

  return 0;
}
