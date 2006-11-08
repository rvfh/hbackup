/* Herve Fache

20061008 Creation
*/

#define _GNU_SOURCE
#include <stdio.h>
#include "filters.c"

static char *filters_show(const void *payload) {
  const filter_t     *filter = payload;
  char               *string = NULL;

  switch (filter->type) {
    case filter_path_end:
    case filter_path_start:
    case filter_path_regexp:
      asprintf(&string, "-> %s %u", filter->string, filter->type);
      break;
    case filter_size_above:
    case filter_size_below:
      asprintf(&string, "-> %u %u", filter->size, filter->type);
      break;
    default:
      asprintf(&string, "-> unknown filter type");
  }
  return string;
}

static char *filters_rule_show(const void *payload) {
  const list_t       rule    = (const list_t) payload;
  char               *string = NULL;

  printf(" Start of rule\n");
  asprintf(&string, "End of rule");
  list_show(rule, NULL, filters_show);
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

  printf("Filters test\n");
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

  /* No filter check possible for size comparison: not a function */

  printf("\nMatch function test\n");
  filter.type            = filter_size_below;
  filter.file_type       = S_IFREG;
  filter.size            = 5000;
  filedata.metadata.size = 4000;
  if (filter_match(&filter, &filedata)) {
    printf("Not matching %lu\n", filedata.metadata.size);
  } else {
    printf("Matching %lu\n", filedata.metadata.size);
  }
  filedata.metadata.size = 6000;
  if (filter_match(&filter, &filedata)) {
    printf("Not matching %lu\n", filedata.metadata.size);
  } else {
    printf("Matching %lu\n", filedata.metadata.size);
  }

  printf("\nSimple rules test\n");
  if (filters_new(&handle)) {
    printf("Failed to create\n");
  } else {
    if (filters_rule_add(filters_rule_new(handle), S_IFREG, filter_path_regexp, "^to a.*\\.txt")) {
      printf("Failed to add\n");
    } else {
      list_show(handle, NULL, filters_rule_show);
    }
    if (filters_rule_add(filters_rule_new(handle), S_IFREG, filter_path_regexp, "^to a.*\\.t.t")) {
      printf("Failed to add\n");
    } else {
      list_show(handle, NULL, filters_rule_show);
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
      if (filters_rule_add(filters_rule_new(handle2), S_IFREG, filter_path_regexp, "^to a.*\\.txt")) {
        printf("Failed to add\n");
      } else {
        list_show(handle2, NULL, filters_rule_show);
      }
      if (filters_rule_add(filters_rule_new(handle2), S_IFREG, filter_path_regexp, "^to a.*\\.t.t")) {
        printf("Failed to add\n");
      } else {
        list_show(handle2, NULL, filters_rule_show);
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
      printf("Not matching %lu\n", filedata.metadata.size);
    } else {
      printf("Matching %lu\n", filedata.metadata.size);
    }
    filedata.metadata.size = 1000;
    if (filters_match(handle, &filedata)) {
      printf("Not matching %lu\n", filedata.metadata.size);
    } else {
      printf("Matching %lu\n", filedata.metadata.size);
    }
    filedata.metadata.size = 1000000;
    if (filters_match(handle, &filedata)) {
      printf("Not matching %lu\n", filedata.metadata.size);
    } else {
      printf("Matching %lu\n", filedata.metadata.size);
    }
    /* File type is always S_IFREG */
    if (filters_rule_add(filters_rule_new(handle), 0, filter_size_below, 500)) {
      printf("Failed to add\n");
    } else {
      list_show(handle, NULL, filters_rule_show);
    }
    filedata.metadata.size = 0;
    if (filters_match(handle, &filedata)) {
      printf("Not matching %lu\n", filedata.metadata.size);
    } else {
      printf("Matching %lu\n", filedata.metadata.size);
    }
    filedata.metadata.size = 1000;
    if (filters_match(handle, &filedata)) {
      printf("Not matching %lu\n", filedata.metadata.size);
    } else {
      printf("Matching %lu\n", filedata.metadata.size);
    }
    filedata.metadata.size = 1000000;
    if (filters_match(handle, &filedata)) {
      printf("Not matching %lu\n", filedata.metadata.size);
    } else {
      printf("Matching %lu\n", filedata.metadata.size);
    }
    /* File type is always S_IFREG */
    if (filters_rule_add(filters_rule_new(handle), 0, filter_size_above, 5000)) {
      printf("Failed to add\n");
    } else {
      list_show(handle, NULL, filters_rule_show);
    }
    filedata.metadata.size = 0;
    if (filters_match(handle, &filedata)) {
      printf("Not matching %lu\n", filedata.metadata.size);
    } else {
      printf("Matching %lu\n", filedata.metadata.size);
    }
    filedata.metadata.size = 1000;
    if (filters_match(handle, &filedata)) {
      printf("Not matching %lu\n", filedata.metadata.size);
    } else {
      printf("Matching %lu\n", filedata.metadata.size);
    }
    filedata.metadata.size = 1000000;
    if (filters_match(handle, &filedata)) {
      printf("Not matching %lu\n", filedata.metadata.size);
    } else {
      printf("Matching %lu\n", filedata.metadata.size);
    }
    filters_free(handle);
  }

  /* Test complex rules */
  printf("\nComplex rules test\n");
  if (filters_new(&handle)) {
    printf("Failed to create\n");
  } else {
    list_t rule = NULL;

    list_show(handle, NULL, filters_rule_show);

    rule = filters_rule_new(handle);
    if (filters_rule_add(rule, 0, filter_size_below, 500)) {
      printf("Failed to add\n");
    } else {
      list_show(handle, NULL, filters_rule_show);
    }
    if (filters_rule_add(rule, 0, filter_size_above, 400)) {
      printf("Failed to add\n");
    } else {
      list_show(handle, NULL, filters_rule_show);
    }
    filedata.metadata.size = 600;
    if (filters_match(handle, &filedata)) {
      printf("Not matching %lu\n", filedata.metadata.size);
    } else {
      printf("Matching %lu\n", filedata.metadata.size);
    }
    filedata.metadata.size = 500;
    if (filters_match(handle, &filedata)) {
      printf("Not matching %lu\n", filedata.metadata.size);
    } else {
      printf("Matching %lu\n", filedata.metadata.size);
    }
    filedata.metadata.size = 450;
    if (filters_match(handle, &filedata)) {
      printf("Not matching %lu\n", filedata.metadata.size);
    } else {
      printf("Matching %lu\n", filedata.metadata.size);
    }
    filedata.metadata.size = 400;
    if (filters_match(handle, &filedata)) {
      printf("Not matching %lu\n", filedata.metadata.size);
    } else {
      printf("Matching %lu\n", filedata.metadata.size);
    }
    filedata.metadata.size = 300;
    if (filters_match(handle, &filedata)) {
      printf("Not matching %lu\n", filedata.metadata.size);
    } else {
      printf("Matching %lu\n", filedata.metadata.size);
    }

    filters_free(handle);
  }

  return 0;
}
