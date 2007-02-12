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

using namespace std;

#include <iostream>
#include "metadata.h"
#include "common.h"
#include "filters.h"

void filter_show(const Filter& filter) {
  /* Read through list of rules */
  for (unsigned int i = 0; i < filter.size(); i++) {
    Rule *rule = filter[i];

    cout << "-> List " << rule->size() << " condition(s)\n";
    /* Read through list of conditions in rule */
    for (unsigned int j = 0; j < rule->size(); j++) {
      Condition *condition = (*rule)[j];

      condition->show();
    }
  }
}

/* TODO Test file type check */
int main(void) {
  Filter      *filter = NULL;
  Filter      *filter2 = NULL;
  Condition   *condition;
  filedata_t  filedata;

  filedata.metadata.type = S_IFREG;

  cout << "Conditions test\n";
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
  filter->push_back(new Rule(new Condition(S_IFREG, filter_path_regexp, "^to a.*\\.txt")));
  cout << ">List " << filter->size() << " rule(s):\n";
  filter_show(*filter);

  filter->push_back(new Rule(new Condition(S_IFREG, filter_path_regexp, "^to a.*\\.t.t")));
  cout << ">List " << filter->size() << " rule(s):\n";
  filter_show(*filter);

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
  filter2->push_back(new Rule(new Condition(S_IFREG, filter_path_regexp, "^to a.*\\.txt")));
  cout << ">List " << filter2->size() << " rule(s):\n";
  filter_show(*filter2);

  filter2->push_back(new Rule(new Condition(S_IFREG, filter_path_regexp, "^to a.*\\.t.t")));
  cout << ">List " << filter2->size() << " rule(s):\n";
  filter_show(*filter2);

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
  filter->push_back(new Rule(new Condition(0, filter_size_below, 500)));
  cout << ">List " << filter->size() << " rule(s):\n";
  filter_show(*filter);

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
  filter->push_back(new Rule(new Condition(0, filter_size_above, 5000)));
  cout << ">List " << filter->size() << " rule(s):\n";
  filter_show(*filter);

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
  Rule *rule = new Rule;

  cout << ">List " << filter->size() << " rule(s):\n";
  filter_show(*filter);

  filter->push_back(rule);

  rule->push_back(new Condition(0, filter_size_below, 500));
  cout << ">List " << filter->size() << " rule(s):\n";
  filter_show(*filter);

  rule->push_back(new Condition(0, filter_size_above, 400));
  cout << ">List " << filter->size() << " rule(s):\n";
  filter_show(*filter);

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
