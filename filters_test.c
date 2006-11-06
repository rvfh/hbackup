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

  switch (filter->type) {
    case filter_path_end:
    case filter_path_start:
    case filter_path_regexp:
      asprintf(&string, "%s %u", filter->string, filter->type);
      break;
    case filter_size_above:
    case filter_size_below:
      asprintf(&string, "%u %u", filter->size, filter->type);
      break;
    default:
      asprintf(&string, "unknown filter type");
  }
  return string;
}

/* TODO Test file type check */
int main(void) {
  void       *handle = NULL;
  void       *handle2 = NULL;
  filedata_t filedata;
  filter_t   filter;

  filedata.metadata.type = S_IFREG;
  filter.file_type = S_IFREG;

  strcpy(filter.string, ".txt");
  printf("filter_path_end_check\n");
  if (! filter_path_end_check("to a file.txt", &filter)) {
    printf("match 1.1\n");
  }
  if (! filter_path_end_check("to a file.tst", &filter)) {
    printf("match 1.2\n");
  }

  printf("filter_path_start_check\n");
  strcpy(filter.string, "this is/a");
  if (! filter_path_start_check("this is/a path/to a file.txt", &filter)) {
    printf("match 2.1\n");
  }
  strcpy(filter.string, "this was/a");
  if (! filter_path_start_check("this is/a path/to a file.txt", &filter)) {
    printf("match 2.2\n");
  }

  printf("filter_path_regexp_check\n");
  strcpy(filter.string, "^this.*path/.*\\.txt");
  if (! filter_path_regexp_check("this is/a path/to a file.txt", &filter)) {
    printf("match 3.1\n");
  }
  strcpy(filter.string, "^this.*path/a.*\\.txt");
  if (! filter_path_regexp_check("this is/a path/to a file.txt", &filter)) {
    printf("match 3.2\n");
  }

  if (filters_new(&handle)) {
    printf("Failed to create\n");
  } else {
    if (filters_add(handle, S_IFREG, filter_path_regexp, "^to a.*\\.txt")) {
      printf("Failed to add\n");
    } else {
      list_show(handle, NULL, filters_show);
    }
    if (filters_add(handle, S_IFREG, filter_path_regexp, "^to a.*\\.t.t")) {
      printf("Failed to add\n");
    } else {
      list_show(handle, NULL, filters_show);
    }
    filedata.path = "to a file.txt";
    if (filters_match(handle, &filedata)) {
      printf("Not matching 1\n");
    }
    filedata.path = "to a file.tst";
    if (filters_match(handle, &filedata)) {
      printf("Not matching 2\n");
    }
    filedata.path = "to a file.tsu";
    if (filters_match(handle, &filedata)) {
      printf("Not matching 3\n");
    }

    if (filters_new(&handle2)) {
      printf("Failed to create\n");
    } else {
      filedata.path = "to a file.txt";
      if (filters_match(handle2, &filedata)) {
        printf("Not matching +1\n");
      }
      filedata.path = "to a file.tst";
      if (filters_match(handle2, &filedata)) {
        printf("Not matching +2\n");
      }
      filedata.path = "to a file.tsu";
      if (filters_match(handle2, &filedata)) {
        printf("Not matching +3\n");
      }
      if (filters_add(handle2, S_IFREG, filter_path_regexp, "^to a.*\\.txt")) {
        printf("Failed to add\n");
      } else {
        list_show(handle2, NULL, filters_show);
      }
      if (filters_add(handle2, S_IFREG, filter_path_regexp, "^to a.*\\.t.t")) {
        printf("Failed to add\n");
      } else {
        list_show(handle2, NULL, filters_show);
      }
      filedata.path = "to a file.txt";
      if (filters_match(handle2, &filedata)) {
        printf("Not matching +1\n");
      }
      filedata.path = "to a file.tst";
      if (filters_match(handle2, &filedata)) {
        printf("Not matching +2\n");
      }
      filedata.path = "to a file.tsu";
      if (filters_match(handle2, &filedata)) {
        printf("Not matching +3\n");
      }
      filters_free(handle2);
    }

    filedata.path = "to a file.txt";
    if (filters_match(handle, &filedata)) {
      printf("Not matching 1\n");
    }
    filedata.path = "to a file.tst";
    if (filters_match(handle, &filedata)) {
      printf("Not matching 2\n");
    }
    filedata.path = "to a file.tsu";
    if (filters_match(handle, &filedata)) {
      printf("Not matching 3\n");
    }
    filters_free(handle);
  }

  if (filters_new(&handle)) {
    printf("Failed to create\n");
  } else {
    filedata.metadata.size = 0;
    if (filters_match(handle, &filedata)) {
      printf("Not matching +1\n");
    }
    filedata.metadata.size = 1000;
    if (filters_match(handle, &filedata)) {
      printf("Not matching +2\n");
    }
    filedata.metadata.size = 1000000;
    if (filters_match(handle, &filedata)) {
      printf("Not matching +3\n");
    }
    /* File type is always S_IFREG */
    if (filters_add(handle, 0, filter_size_below, 500)) {
      printf("Failed to add\n");
    } else {
      list_show(handle, NULL, filters_show);
    }
    filedata.metadata.size = 0;
    if (filters_match(handle, &filedata)) {
      printf("Not matching +1\n");
    }
    filedata.metadata.size = 1000;
    if (filters_match(handle, &filedata)) {
      printf("Not matching +2\n");
    }
    filedata.metadata.size = 1000000;
    if (filters_match(handle, &filedata)) {
      printf("Not matching +3\n");
    }
    /* File type is always S_IFREG */
    if (filters_add(handle, 0, filter_size_above, 5000)) {
      printf("Failed to add\n");
    } else {
      list_show(handle, NULL, filters_show);
    }
    filedata.metadata.size = 0;
    if (filters_match(handle, &filedata)) {
      printf("Not matching +1\n");
    }
    filedata.metadata.size = 1000;
    if (filters_match(handle, &filedata)) {
      printf("Not matching +2\n");
    }
    filedata.metadata.size = 1000000;
    if (filters_match(handle, &filedata)) {
      printf("Not matching +3\n");
    }
    filters_free(handle);
  }

  return 0;
}
