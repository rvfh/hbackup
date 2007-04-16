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

// TODO test file type filter

using namespace std;

#include <iostream>
#include <vector>
#include <sys/stat.h>

#include "files.h"
#include "filters.h"

int terminating(void) {
  return 0;
}

void filter_show(const Filters& filters) {
  /* Read through list of filters */
  for (unsigned int i = 0; i < filters.size(); i++) {
    Filter filter = filters[i];

    cout << "-> List " << filter.size() << " condition(s)\n";
    /* Read through list of conditions in filter */
    for (unsigned int j = 0; j < filter.size(); j++) {
      Condition condition = filter[j];

      condition.show();
    }
  }
}

/* TODO Test file type check */
int main(void) {
  Filters   *filter = NULL;
  Filters   *filter2 = NULL;
  Condition *condition;
  File      *file_data;

  cout << "Conditions test\n";
  cout << "filter_path_end_check\n";
  condition = new Condition(filter_path_end, ".txt");
  file_data = new File("", "to a file.txt", "", S_IFREG, 0, 0, 0, 0, 0);
  if (! condition->match(*file_data)) {
    cout << "match 1.1\n";
  }
  delete file_data;
  file_data = new File("", "to a file.tst", "", S_IFREG, 0, 0, 0, 0, 0);
  if (! condition->match(*file_data)) {
    cout << "match 1.2\n";
  }
  delete file_data;
  delete condition;

  cout << "filter_path_start_check\n";
  file_data = new File("", "this is/a path/to a file.txt", "", S_IFREG, 0, 0, 0, 0, 0);
  condition = new Condition(filter_path_start, "this is/a");

  if (! condition->match(*file_data)) {
    cout << "match 2.1\n";
  }
  delete condition;
  condition = new Condition(filter_path_start, "this was/a");
  if (! condition->match(*file_data)) {
    cout << "match 2.2\n";
  }
  delete condition;

  cout << "filter_path_regexp_check\n";
  condition = new Condition(filter_path_regexp, "^this.*path/.*\\.txt");
  if (! condition->match(*file_data)) {
    cout << "match 3.1\n";
  }
  delete condition;
  condition = new Condition(filter_path_regexp, "^this.*path/a.*\\.txt");
  if (! condition->match(*file_data)) {
    cout << "match 3.2\n";
  }
  delete condition;
  delete file_data;


  cout << "\nMatch function test\n";
  condition = new Condition(filter_size_below, (off_t) 5000);
  file_data = new File("", "", "", S_IFREG, 0, 4000, 0, 0, 0);
  if (condition->match(*file_data)) {
    cout << "Not matching " << file_data->size() << "\n";
  } else {
    cout << "Matching " << file_data->size() << "\n";
  }
  delete file_data;
  file_data = new File("", "", "", S_IFREG, 0, 6000, 0, 0, 0);
  if (condition->match(*file_data)) {
    cout << "Not matching " << file_data->size() << "\n";
  } else {
    cout << "Matching " << file_data->size() << "\n";
  }
  delete file_data;
  delete condition;


  cout << "\nSimple rules test\n";
  filter = new Filters;
  filter->push_back(Filter(Condition(filter_path_regexp, "^to a.*\\.txt")));
  cout << ">List " << filter->size() << " rule(s):\n";
  filter_show(*filter);

  filter->push_back(Filter(Condition(filter_path_regexp, "^to a.*\\.t.t")));
  cout << ">List " << filter->size() << " rule(s):\n";
  filter_show(*filter);

  file_data = new File("", "to a file.txt", "", S_IFREG, 0, 0, 0, 0, 0);
  if (filter->match(*file_data)) {
    cout << "Not matching 1\n";
  }
  delete file_data;
  file_data = new File("", "to a file.tst", "", S_IFREG, 0, 0, 0, 0, 0);
  if (filter->match(*file_data)) {
    cout << "Not matching 2\n";
  }
  delete file_data;
  file_data = new File("", "to a file.tsu", "", S_IFREG, 0, 0, 0, 0, 0);
  if (filter->match(*file_data)) {
    cout << "Not matching 3\n";
  }
  delete file_data;

  filter2 = new Filters;
  file_data = new File("", "to a file.txt", "", S_IFREG, 0, 0, 0, 0, 0);
  if (filter2->match(*file_data)) {
    cout << "Not matching +1\n";
  }
  delete file_data;
  file_data = new File("", "to a file.tst", "", S_IFREG, 0, 0, 0, 0, 0);
  if (filter2->match(*file_data)) {
    cout << "Not matching +2\n";
  }
  delete file_data;
  file_data = new File("", "to a file.tsu", "", S_IFREG, 0, 0, 0, 0, 0);
  if (filter2->match(*file_data)) {
    cout << "Not matching +3\n";
  }
  delete file_data;


  filter2->push_back(Filter(Condition(filter_path_regexp, "^to a.*\\.txt")));
  cout << ">List " << filter2->size() << " rule(s):\n";
  filter_show(*filter2);

  filter2->push_back(Filter(Condition(filter_path_regexp, "^to a.*\\.t.t")));
  cout << ">List " << filter2->size() << " rule(s):\n";
  filter_show(*filter2);

  file_data = new File("", "to a file.txt", "", S_IFREG, 0, 0, 0, 0, 0);
  if (filter2->match(*file_data)) {
    cout << "Not matching +1\n";
  }
  delete file_data;
  file_data = new File("", "to a file.tst", "", S_IFREG, 0, 0, 0, 0, 0);
  if (filter2->match(*file_data)) {
    cout << "Not matching +2\n";
  }
  delete file_data;
  file_data = new File("", "to a file.tsu", "", S_IFREG, 0, 0, 0, 0, 0);
  if (filter2->match(*file_data)) {
    cout << "Not matching +3\n";
  }
  delete file_data;
  delete filter2;

  file_data = new File("", "to a file.txt", "", S_IFREG, 0, 0, 0, 0, 0);
  if (filter->match(*file_data)) {
    cout << "Not matching 1\n";
  }
  delete file_data;
  file_data = new File("", "to a file.tst", "", S_IFREG, 0, 0, 0, 0, 0);
  if (filter->match(*file_data)) {
    cout << "Not matching 2\n";
  }
  delete file_data;
  file_data = new File("", "to a file.tsu", "", S_IFREG, 0, 0, 0, 0, 0);
  if (filter->match(*file_data)) {
    cout << "Not matching 3\n";
  }
  delete file_data;
  delete filter;

  filter = new Filters;
  file_data = new File("", "", "", S_IFREG, 0, 0, 0, 0, 0);
  if (filter->match(*file_data)) {
    cout << "Not matching " << file_data->size() << "\n";
  } else {
    cout << "Matching " << file_data->size() << "\n";
  }
  delete file_data;
  file_data = new File("", "", "", S_IFREG, 0, 1000, 0, 0, 0);
  if (filter->match(*file_data)) {
    cout << "Not matching " << file_data->size() << "\n";
  } else {
    cout << "Matching " << file_data->size() << "\n";
  }
  delete file_data;
  file_data = new File("", "", "", S_IFREG, 0, 1000000, 0, 0, 0);
  if (filter->match(*file_data)) {
    cout << "Not matching " << file_data->size() << "\n";
  } else {
    cout << "Matching " << file_data->size() << "\n";
  }
  delete file_data;

  /* File type is always S_IFREG */
  filter->push_back(Filter(Condition(filter_size_below, (off_t) 500)));
  cout << ">List " << filter->size() << " rule(s):\n";
  filter_show(*filter);

  file_data = new File("", "", "", S_IFREG, 0, 0, 0, 0, 0);
  if (filter->match(*file_data)) {
    cout << "Not matching " << file_data->size() << "\n";
  } else {
    cout << "Matching " << file_data->size() << "\n";
  }
  delete file_data;
  file_data = new File("", "", "", S_IFREG, 0, 1000, 0, 0, 0);
  if (filter->match(*file_data)) {
    cout << "Not matching " << file_data->size() << "\n";
  } else {
    cout << "Matching " << file_data->size() << "\n";
  }
  delete file_data;
  file_data = new File("", "", "", S_IFREG, 0, 1000000, 0, 0, 0);
  if (filter->match(*file_data)) {
    cout << "Not matching " << file_data->size() << "\n";
  } else {
    cout << "Matching " << file_data->size() << "\n";
  }
  delete file_data;

  /* File type is always S_IFREG */
  filter->push_back(Filter(Condition(filter_size_above, (off_t) 5000)));
  cout << ">List " << filter->size() << " rule(s):\n";
  filter_show(*filter);

  file_data = new File("", "", "", S_IFREG, 0, 0, 0, 0, 0);
  if (filter->match(*file_data)) {
    cout << "Not matching " << file_data->size() << "\n";
  } else {
    cout << "Matching " << file_data->size() << "\n";
  }
  delete file_data;
  file_data = new File("", "", "", S_IFREG, 0, 1000, 0, 0, 0);
  if (filter->match(*file_data)) {
    cout << "Not matching " << file_data->size() << "\n";
  } else {
    cout << "Matching " << file_data->size() << "\n";
  }
  delete file_data;
  file_data = new File("", "", "", S_IFREG, 0, 1000000, 0, 0, 0);
  if (filter->match(*file_data)) {
    cout << "Not matching " << file_data->size() << "\n";
  } else {
    cout << "Matching " << file_data->size() << "\n";
  }
  delete file_data;

  delete filter;

  /* Test complex rules */
  cout << "\nComplex rules test\n";
  filter = new Filters;

  cout << ">List " << filter->size() << " rule(s):\n";
  filter_show(*filter);

  filter->push_back(Filter());
  Filter *rule = &(*filter)[filter->size() - 1];

  rule->push_back(Condition(filter_size_below, (off_t) 500));
  cout << ">List " << filter->size() << " rule(s):\n";
  filter_show(*filter);

  rule->push_back(Condition(filter_size_above, (off_t) 400));
  cout << ">List " << filter->size() << " rule(s):\n";
  filter_show(*filter);

  file_data = new File("", "", "", S_IFREG, 0, 600, 0, 0, 0);
  if (filter->match(*file_data)) {
    cout << "Not matching " << file_data->size() << "\n";
  } else {
    cout << "Matching " << file_data->size() << "\n";
  }
  delete file_data;
  file_data = new File("", "", "", S_IFREG, 0, 500, 0, 0, 0);
  if (filter->match(*file_data)) {
    cout << "Not matching " << file_data->size() << "\n";
  } else {
    cout << "Matching " << file_data->size() << "\n";
  }
  delete file_data;
  file_data = new File("", "", "", S_IFREG, 0, 450, 0, 0, 0);
  if (filter->match(*file_data)) {
    cout << "Not matching " << file_data->size() << "\n";
  } else {
    cout << "Matching " << file_data->size() << "\n";
  }
  delete file_data;
  file_data = new File("", "", "", S_IFREG, 0, 400, 0, 0, 0);
  if (filter->match(*file_data)) {
    cout << "Not matching " << file_data->size() << "\n";
  } else {
    cout << "Matching " << file_data->size() << "\n";
  }
  delete file_data;
  file_data = new File("", "", "", S_IFREG, 0, 300, 0, 0, 0);
  if (filter->match(*file_data)) {
    cout << "Not matching " << file_data->size() << "\n";
  } else {
    cout << "Matching " << file_data->size() << "\n";
  }
  delete file_data;

  delete filter;

  return 0;
}