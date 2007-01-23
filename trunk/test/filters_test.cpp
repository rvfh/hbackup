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
  const filter_t     *filter = (const filter_t *) (payload);
  char               *string = NULL;

  switch (filter->type) {
    case filter_path_end:
    case filter_path_start:
    case filter_path_regexp:
      asprintf(&string, "%s %u", filter->string, filter->type);
      break;
    case filter_size_above:
    case filter_size_below:
      asprintf(&string, "%ld %u", filter->size, filter->type);
      break;
    default:
      asprintf(&string, "unknown filter type");
  }
  return string;
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
  List       *handle = NULL;
  List       *handle2 = NULL;
  filedata_t filedata;
  filter_t   filter;

  filedata.metadata.type = S_IFREG;
  filter.file_type = S_IFREG;

  cout << "Filters test\n";
  strcpy(filter.string, ".txt");
  cout << "filter_path_end_check\n";
  if (! filter_path_end_check("to a file.txt", &filter)) {
    cout << "match 1.1\n";
  }
  if (! filter_path_end_check("to a file.tst", &filter)) {
    cout << "match 1.2\n";
  }

  cout << "filter_path_start_check\n";
  strcpy(filter.string, "this is/a");
  if (! filter_path_start_check("this is/a path/to a file.txt", &filter)) {
    cout << "match 2.1\n";
  }
  strcpy(filter.string, "this was/a");
  if (! filter_path_start_check("this is/a path/to a file.txt", &filter)) {
    cout << "match 2.2\n";
  }

  cout << "filter_path_regexp_check\n";
  strcpy(filter.string, "^this.*path/.*\\.txt");
  if (! filter_path_regexp_check("this is/a path/to a file.txt", &filter)) {
    cout << "match 3.1\n";
  }
  strcpy(filter.string, "^this.*path/a.*\\.txt");
  if (! filter_path_regexp_check("this is/a path/to a file.txt", &filter)) {
    cout << "match 3.2\n";
  }

  /* No filter check possible for size comparison: not a function */

  cout << "\nMatch function test\n";
  filter.type            = filter_size_below;
  filter.file_type       = S_IFREG;
  filter.size            = 5000;
  filedata.metadata.size = 4000;
  if (filter_match(&filter, &filedata)) {
    cout << "Not matching " << filedata.metadata.size << "\n";
  } else {
    cout << "Matching " << filedata.metadata.size << "\n";
  }
  filedata.metadata.size = 6000;
  if (filter_match(&filter, &filedata)) {
    cout << "Not matching " << filedata.metadata.size << "\n";
  } else {
    cout << "Matching " << filedata.metadata.size << "\n";
  }

  cout << "\nSimple rules test\n";
  if (filters_new(&handle)) {
    cout << "Failed to create\n";
  } else {
    if (filters_rule_add(filters_rule_new(handle), S_IFREG, filter_path_regexp, "^to a.*\\.txt")) {
      cout << "Failed to add\n";
    } else {
      cout << ">List " << handle->size() << " rule(s):\n";
      handle->show(NULL, filters_rule_show);
    }
    if (filters_rule_add(filters_rule_new(handle), S_IFREG, filter_path_regexp, "^to a.*\\.t.t")) {
      cout << "Failed to add\n";
    } else {
      cout << ">List " << handle->size() << " rule(s):\n";
      handle->show(NULL, filters_rule_show);
    }
    filedata.path = "to a file.txt";
    if (filters_match(handle, &filedata)) {
      cout << "Not matching 1\n";
    }
    filedata.path = "to a file.tst";
    if (filters_match(handle, &filedata)) {
      cout << "Not matching 2\n";
    }
    filedata.path = "to a file.tsu";
    if (filters_match(handle, &filedata)) {
      cout << "Not matching 3\n";
    }

    if (filters_new(&handle2)) {
      cout << "Failed to create\n";
    } else {
      filedata.path = "to a file.txt";
      if (filters_match(handle2, &filedata)) {
        cout << "Not matching +1\n";
      }
      filedata.path = "to a file.tst";
      if (filters_match(handle2, &filedata)) {
        cout << "Not matching +2\n";
      }
      filedata.path = "to a file.tsu";
      if (filters_match(handle2, &filedata)) {
        cout << "Not matching +3\n";
      }
      if (filters_rule_add(filters_rule_new(handle2), S_IFREG, filter_path_regexp, "^to a.*\\.txt")) {
        cout << "Failed to add\n";
      } else {
        cout << ">List " << handle2->size() << " rule(s):\n";
        handle2->show(NULL, filters_rule_show);
      }
      if (filters_rule_add(filters_rule_new(handle2), S_IFREG, filter_path_regexp, "^to a.*\\.t.t")) {
        cout << "Failed to add\n";
      } else {
        cout << ">List " << handle2->size() << " rule(s):\n";
        handle2->show(NULL, filters_rule_show);
      }
      filedata.path = "to a file.txt";
      if (filters_match(handle2, &filedata)) {
        cout << "Not matching +1\n";
      }
      filedata.path = "to a file.tst";
      if (filters_match(handle2, &filedata)) {
        cout << "Not matching +2\n";
      }
      filedata.path = "to a file.tsu";
      if (filters_match(handle2, &filedata)) {
        cout << "Not matching +3\n";
      }
      filters_free(handle2);
    }

    filedata.path = "to a file.txt";
    if (filters_match(handle, &filedata)) {
      cout << "Not matching 1\n";
    }
    filedata.path = "to a file.tst";
    if (filters_match(handle, &filedata)) {
      cout << "Not matching 2\n";
    }
    filedata.path = "to a file.tsu";
    if (filters_match(handle, &filedata)) {
      cout << "Not matching 3\n";
    }
    filters_free(handle);
  }

  if (filters_new(&handle)) {
    cout << "Failed to create\n";
  } else {
    filedata.metadata.size = 0;
    if (filters_match(handle, &filedata)) {
      cout << "Not matching " << filedata.metadata.size << "\n";
    } else {
      cout << "Matching " << filedata.metadata.size << "\n";
    }
    filedata.metadata.size = 1000;
    if (filters_match(handle, &filedata)) {
      cout << "Not matching " << filedata.metadata.size << "\n";
    } else {
      cout << "Matching " << filedata.metadata.size << "\n";
    }
    filedata.metadata.size = 1000000;
    if (filters_match(handle, &filedata)) {
      cout << "Not matching " << filedata.metadata.size << "\n";
    } else {
      cout << "Matching " << filedata.metadata.size << "\n";
    }
    /* File type is always S_IFREG */
    if (filters_rule_add(filters_rule_new(handle), 0, filter_size_below, 500)) {
      cout << "Failed to add\n";
    } else {
      cout << ">List " << handle->size() << " rule(s):\n";
      handle->show(NULL, filters_rule_show);
    }
    filedata.metadata.size = 0;
    if (filters_match(handle, &filedata)) {
      cout << "Not matching " << filedata.metadata.size << "\n";
    } else {
      cout << "Matching " << filedata.metadata.size << "\n";
    }
    filedata.metadata.size = 1000;
    if (filters_match(handle, &filedata)) {
      cout << "Not matching " << filedata.metadata.size << "\n";
    } else {
      cout << "Matching " << filedata.metadata.size << "\n";
    }
    filedata.metadata.size = 1000000;
    if (filters_match(handle, &filedata)) {
      cout << "Not matching " << filedata.metadata.size << "\n";
    } else {
      cout << "Matching " << filedata.metadata.size << "\n";
    }
    /* File type is always S_IFREG */
    if (filters_rule_add(filters_rule_new(handle), 0, filter_size_above, 5000)) {
      cout << "Failed to add\n";
    } else {
      cout << ">List " << handle->size() << " rule(s):\n";
      handle->show(NULL, filters_rule_show);
    }
    filedata.metadata.size = 0;
    if (filters_match(handle, &filedata)) {
      cout << "Not matching " << filedata.metadata.size << "\n";
    } else {
      cout << "Matching " << filedata.metadata.size << "\n";
    }
    filedata.metadata.size = 1000;
    if (filters_match(handle, &filedata)) {
      cout << "Not matching " << filedata.metadata.size << "\n";
    } else {
      cout << "Matching " << filedata.metadata.size << "\n";
    }
    filedata.metadata.size = 1000000;
    if (filters_match(handle, &filedata)) {
      cout << "Not matching " << filedata.metadata.size << "\n";
    } else {
      cout << "Matching " << filedata.metadata.size << "\n";
    }
    filters_free(handle);
  }

  /* Test complex rules */
  cout << "\nComplex rules test\n";
  if (filters_new(&handle)) {
    cout << "Failed to create\n";
  } else {
    List *rule = NULL;

    cout << ">List " << handle->size() << " rule(s):\n";
    handle->show(NULL, filters_rule_show);

    rule = filters_rule_new(handle);
    if (filters_rule_add(rule, 0, filter_size_below, 500)) {
      cout << "Failed to add\n";
    } else {
      cout << ">List " << handle->size() << " rule(s):\n";
      handle->show(NULL, filters_rule_show);
    }
    if (filters_rule_add(rule, 0, filter_size_above, 400)) {
      cout << "Failed to add\n";
    } else {
      cout << ">List " << handle->size() << " rule(s):\n";
      handle->show(NULL, filters_rule_show);
    }
    filedata.metadata.size = 600;
    if (filters_match(handle, &filedata)) {
      cout << "Not matching " << filedata.metadata.size << "\n";
    } else {
      cout << "Matching " << filedata.metadata.size << "\n";
    }
    filedata.metadata.size = 500;
    if (filters_match(handle, &filedata)) {
      cout << "Not matching " << filedata.metadata.size << "\n";
    } else {
      cout << "Matching " << filedata.metadata.size << "\n";
    }
    filedata.metadata.size = 450;
    if (filters_match(handle, &filedata)) {
      cout << "Not matching " << filedata.metadata.size << "\n";
    } else {
      cout << "Matching " << filedata.metadata.size << "\n";
    }
    filedata.metadata.size = 400;
    if (filters_match(handle, &filedata)) {
      cout << "Not matching " << filedata.metadata.size << "\n";
    } else {
      cout << "Matching " << filedata.metadata.size << "\n";
    }
    filedata.metadata.size = 300;
    if (filters_match(handle, &filedata)) {
      cout << "Not matching " << filedata.metadata.size << "\n";
    } else {
      cout << "Matching " << filedata.metadata.size << "\n";
    }

    filters_free(handle);
  }

  return 0;
}
