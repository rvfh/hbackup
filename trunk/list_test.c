/* Herve Fache

20061003 Creation
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "list.h"

typedef struct {
  char name[256];
} payload_t;

typedef struct {
  char dir[256];
  char filename[256];
} payload2_t;

static void payload_get(const void *payload_p, char *string) {
  const payload_t *payload = payload_p;
  sprintf(string, "%s", payload->name);
}

static void payload2_get(const void *payload_p, char *string) {
  const payload2_t  *payload = payload_p;

  sprintf(string, "%s/%s", payload->dir, payload->filename);
}

static int payloads_compare(void *payload_left, void *payload_right) {
  static int counter = 0;

  if (payload_left == NULL) return 1;
  if (payload_right == NULL) return -1;

  if (++counter > 2) counter = -2;
  return counter;
}

static payload_t *list_test_new(payload_t payload) {
  payload_t *payload_p = (payload_t *) malloc(sizeof(payload_t));
  *payload_p = payload;
  return payload_p;
}

static payload2_t *list2_test_new(payload2_t payload2) {
  payload2_t *payload2_p = (payload2_t *) malloc(sizeof(payload2_t));
  *payload2_p = payload2;
  return payload2_p;
}

int main() {
  list_entry_t *entry = NULL;
  payload_t payload;
  payload2_t payload2;
  list_t list = list_new(payload_get);
  list_t list2 = list_new(payload2_get);
  list_t added = NULL;
  list_t missing = NULL;
  char   string[FILENAME_MAX];

  printf("Fill in list\n");
  strcpy(payload.name, "test");
  list_add(list, list_test_new(payload));
  list_show(list, NULL, payload_get);

  strcpy(payload.name, "test/testfile");
  list_add(list, list_test_new(payload));
  list_show(list, NULL, payload_get);
  list_find(list, "test", NULL, &entry);
  payload_get(list_entry_payload(entry), string);
  if (strcmp(string, "test")) {
    printf("test not found???\n");
  }

  strcpy(payload.name, "test/subdir/testfile1");
  list_add(list, list_test_new(payload));
  list_show(list, NULL, payload_get);

  strcpy(payload.name, "test/subdir");
  list_add(list, list_test_new(payload));
  list_show(list, NULL, payload_get);

  strcpy(payload.name, "test/testfile");
  list_add(list, list_test_new(payload));
  list_show(list, NULL, payload_get);
  printf("Reported size: %u\n", list_size(list));

  list_find(list, "test/testfile", NULL, &entry);

  printf("\nFill in list2\n");
  strcpy(payload2.dir, "test");
  strcpy(payload2.filename, "testfile");
  list_add(list2, list2_test_new(payload2));
  list_show(list2, NULL, NULL);

  strcpy(payload2.dir, "test/subdir");
  strcpy(payload2.filename, "testfile1");
  list_add(list2, list2_test_new(payload2));
  list_show(list2, NULL, NULL);

  strcpy(payload2.filename, "added");
  list_add(list2, list2_test_new(payload2));
  list_show(list2, NULL, NULL);

  printf("\nCompare lists\n");
  if (list_compare(list, list2, NULL, NULL, NULL)) {
    printf("Lists differ\n");
  }
  if (list_compare(list, list2, &added, NULL, NULL)) {
    printf("Added list only\n");
    list_show(added, NULL, NULL);
  }
  if (list_compare(list, list2, NULL, &missing, NULL)) {
    printf("Missing list only\n");
    list_show(missing, NULL, NULL);
  }
  if (list_compare(list, list2, &added, &missing, NULL)) {
    printf("Both lists\n");
    list_show(added, NULL, NULL);
    list_show(missing, NULL, NULL);
  }
  if (list_compare(list, list2, &added, &missing, payloads_compare)) {
    printf("Both lists with comparison function\n");
    list_show(added, NULL, NULL);
    list_show(missing, NULL, NULL);
  }

  printf("\nEmpty list\n");
  list_drop(list, entry);
  list_show(list, NULL, payload_get);

  list_drop(list, list_previous(list, NULL));
  list_show(list, NULL, payload_get);

  list_drop(list, list_next(list, NULL));
  list_show(list, NULL, payload_get);

  list_free(list);

  return 0;
}
