/* Herve Fache

20061008 Creation
*/

#define _GNU_SOURCE
#include <stdio.h>
#include "filters.c"

/* Use payload as argument name, cast once and for all */
static char *filters_show(const void *payload) {
  const filter_t *filter = payload;
  char *string = NULL;

  asprintf(&string, "%s %u", filter->string, filter->type);
  return string;
}

int main(void) {
  void *handle = NULL;
  void *handle2 = NULL;

  printf("filter_end_check\n");
  if (! filter_end_check("this is/a path/to a file.txt", ".txt")) {
    printf("match 1.1\n");
  }
  if (! filter_end_check("this is/a path/to a file.tst", ".txt")) {
    printf("match 1.2\n");
  }

  printf("filter_path_start_check\n");
  if (! filter_path_start_check("this is/a path/to a file.txt", "this is/a")) {
    printf("match 2.1\n");
  }
  if (! filter_path_start_check("this is/a path/to a file.txt", "this was/a")) {
    printf("match 2.2\n");
  }

  printf("filter_path_regexp_check\n");
  if (! filter_path_regexp_check("this is/a path/to a file.txt", "^this.*path/.*\\.txt")) {
    printf("match 3.1\n");
  }
  if (! filter_path_regexp_check("this is/a path/to a file.txt", "^this.*path/a.*\\.txt")) {
    printf("match 3.2\n");
  }

  printf("filter_file_start_check\n");
  if (! filter_file_start_check("this is/a path/to a file.txt", "to a file")) {
    printf("match 4.1\n");
  }
  if (! filter_file_start_check("this is/a path/to a file.txt", "to two files")) {
    printf("match 4.2\n");
  }

  printf("filter_file_regexp_check\n");
  if (! filter_file_regexp_check("this is/a path/to a file.txt", "^to a.*\\.txt")) {
    printf("match 5.1\n");
  }
  if (! filter_file_regexp_check("this is/a path/to a file.txt", "^a.*\\.txt")) {
    printf("match 5.2\n");
  }

  if (filters_new(&handle)) {
    printf("Failed to create\n");
  } else {
    if (filters_add(handle, "^to a.*\\.txt", filter_file_regexp)) {
      printf("Failed to add\n");
    } else {
      list_show(handle, NULL, filters_show);
    }
    if (filters_add(handle, "^to a.*\\.t.t", filter_file_regexp)) {
      printf("Failed to add\n");
    } else {
      list_show(handle, NULL, filters_show);
    }
    if (filters_match(handle, "this is/a path/to a file.txt")) {
      printf("Not matching 1\n");
    }
    if (filters_match(handle, "this is/a path/to a file.tst")) {
      printf("Not matching 2\n");
    }
    if (filters_match(handle, "this is/a path/to a file.tsu")) {
      printf("Not matching 3\n");
    }

    if (filters_new(&handle2)) {
      printf("Failed to create\n");
    } else {
      if (filters_match(handle2, "this is/a path/to a file.txt")) {
        printf("Not matching +1\n");
      }
      if (filters_match(handle2, "this is/a path/to a file.tst")) {
        printf("Not matching +2\n");
      }
      if (filters_match(handle2, "this is/a path/to a file.tsu")) {
        printf("Not matching +3\n");
      }
      if (filters_add(handle2, "^to a.*\\.txt", filter_file_regexp)) {
        printf("Failed to add\n");
      } else {
        list_show(handle2, NULL, filters_show);
      }
      if (filters_add(handle2, "^to a.*\\.t.t", filter_file_regexp)) {
        printf("Failed to add\n");
      } else {
        list_show(handle2, NULL, filters_show);
      }
      if (filters_match(handle2, "this is/a path/to a file.txt")) {
        printf("Not matching +1\n");
      }
      if (filters_match(handle2, "this is/a path/to a file.tst")) {
        printf("Not matching +2\n");
      }
      if (filters_match(handle2, "this is/a path/to a file.tsu")) {
        printf("Not matching +3\n");
      }
      filters_free(handle2);
    }

    if (filters_match(handle, "this is/a path/to a file.txt")) {
      printf("Not matching 1\n");
    }
    if (filters_match(handle, "this is/a path/to a file.tst")) {
      printf("Not matching 2\n");
    }
    if (filters_match(handle, "this is/a path/to a file.tsu")) {
      printf("Not matching 3\n");
    }
    filters_free(handle);
  }

  return 0;
}
