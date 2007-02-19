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
#include <string.h>
#include "list.h"

typedef struct {
  char name[256];
} payload_t;

typedef struct {
  char dir[256];
  char filename[256];
} payload2_t;

static char *payload_get(const void *payload_p) {
  const payload_t *payload = (const payload_t *) (payload_p);
  char *string = NULL;

  asprintf(&string, "%s", payload->name);
  return string;
}

static char *payload2_get(const void *payload_p) {
  const payload2_t *payload = (const payload2_t *) (payload_p);
  char *string = NULL;

  asprintf(&string, "%s/%s", payload->dir, payload->filename);
  return string;
}

static int payloads_compare(void *payload_left, void *payload_right) {
  static int counter = 0;

  if (++counter > 2) counter = -2;
  return counter;
}

static payload_t *list_test_new(payload_t payload) {
  payload_t *payload_p = new payload_t;
  *payload_p = payload;
  return payload_p;
}

static payload2_t *list2_test_new(payload2_t payload2) {
  payload2_t *payload2_p = new payload2_t;
  *payload2_p = payload2;
  return payload2_p;
}

int main() {
  list_entry_t  *entry              = NULL;
  payload_t     payload;
  payload2_t    payload2;
  List          list(payload_get);
  List          *list2 = new List(payload2_get);
  List          list3(payload2_get);
  List          *added              = NULL;
  List          *missing            = NULL;
  char          *string             = NULL;

  cout << "Fill in list\n";
  strcpy(payload.name, "test");
  list.add(list_test_new(payload));
  cout << "List " << list.size() << " element(s):\n";
  list.show(NULL, payload_get);

  strcpy(payload.name, "test/testfile");
  list.add(list_test_new(payload));
  cout << "List " << list.size() << " element(s):\n";
  list.show(NULL, payload_get);
  list.find("test", NULL, &entry);
  string = payload_get(list_entry_payload(entry));
  if (strcmp(string, "test")) {
    cout << "test not found???\n";
  }
  free(string);

  strcpy(payload.name, "test/subdir/testfile1");
  list.add(list_test_new(payload));
  cout << "List " << list.size() << " element(s):\n";
  list.show(NULL, payload_get);

  strcpy(payload.name, "test/subdir");
  list.add(list_test_new(payload));
  cout << "List " << list.size() << " element(s):\n";
  list.show(NULL, payload_get);

  strcpy(payload.name, "test/testfile");
  list.add(list_test_new(payload));
  cout << "List " << list.size() << " element(s):\n";
  list.show(NULL, payload_get);
  cout << "Reported size: " << list.size() << "\n";

  list.find("test/testfile", NULL, &entry);

  cout << "\nFill in list2\n";
  strcpy(payload2.dir, "test");
  strcpy(payload2.filename, "testfile");
  list2->add(list2_test_new(payload2));
  cout << "List " << list2->size() << " element(s):\n";
  list2->show();

  strcpy(payload2.dir, "test/subdir");
  strcpy(payload2.filename, "testfile1");
  list2->add(list2_test_new(payload2));
  cout << "List " << list2->size() << " element(s):\n";
  list2->show();

  strcpy(payload2.filename, "added");
  list2->add(list2_test_new(payload2));
  cout << "List " << list2->size() << " element(s):\n";
  list2->show();

  cout << "\nCompare lists\n";
  if (list.compare(list2)) {
    cout << "Lists differ\n";
  }
  added = new List(payload2_get);
  if (list.compare(list2, added)) {
    cout << "Added list only\n";
    cout << "List " << added->size() << " element(s):\n";
    added->show();
    if (added->deselect() != 0) {
      cout << "Added list not empty after deselect\n";
    }
  }
  delete added;
  missing = new List(payload_get);
  if (list.compare(list2, NULL, missing)) {
    cout << "Missing list only\n";
    cout << "List " << missing->size() << " element(s):\n";
    missing->show();
    if (missing->deselect() != 0) {
      cout << "Missing list not freed\n";
    }
  }
  delete missing;
  added = new List(payload2_get);
  missing = new List(payload_get);
  if (list.compare(list2, added, missing)) {
    cout << "Both lists\n";
    cout << "List " << added->size() << " element(s):\n";
    added->show();
    if (added->deselect() != 0) {
      cout << "Added list not freed\n";
    }
    cout << "List " << missing->size() << " element(s):\n";
    missing->show();
    if (missing->deselect() != 0) {
      cout << "Missing list not freed\n";
    }
  }
  delete added;
  delete missing;
  added = new List(payload2_get);
  missing = new List(payload_get);
  if (list.compare(list2, added, missing, payloads_compare)) {
    cout << "Both lists with comparison function\n";
    cout << "List " << added->size() << " element(s):\n";
    added->show();
    if (added->deselect() != 0) {
      cout << "Added list not freed\n";
    }
    cout << "List " << missing->size() << " element(s):\n";
    missing->show();
    if (missing->deselect() != 0) {
      cout << "Missing list not freed\n";
    }
  }
  delete added;
  delete missing;
  cout << "List " << list2->size() << " element(s) of original list:\n";
  list2->show();

  cout << "\nSelect part of list\n";
  cout << "List " << list.size() << " element(s) of original list:\n";
  list.show(NULL, payload_get);
  List *list_s1 = new List(payload_get);
  List *list_u1 = new List(payload_get);
  if (list.select("test/subdir/", NULL, list_s1, list_u1)) {
    cout << "Select failed\n";
  } else {
    cout << "List " << list_s1->size() << " element(s) of selected list:\n";
    list_s1->show();
    if (list_s1->deselect() != 0) {
      cout << "Selected list not freed\n";
    }
    cout << "List " << list_u1->size() << " element(s) of unselected list:\n";
    list_u1->show();
    if (list_u1->deselect() != 0) {
      cout << "Selected list not freed\n";
    }
  }
  delete list_s1;
  delete list_u1;
  List *list_s2 = new List(payload_get);
  if (list.select("test/subdir/", NULL, list_s2, NULL)) {
    cout << "Select failed\n";
  } else {
    cout << "List " << list_s2->size() << " element(s) of selected list:\n";
    list_s2->show();
    if (list_s2->deselect() != 0) {
      cout << "Selected list not freed\n";
    }
  }
  delete list_s2;
  List *list_u2 = new List(payload_get);
  if (list.select("test/subdir/", NULL, NULL, list_u2)) {
    cout << "Select failed\n";
  } else {
    cout << "List " << list_u2->size() << " element(s) of unselected list:\n";
    list_u2->show();
    if (list_u2->deselect() != 0) {
      cout << "Selected list not freed\n";
    }
  }
  delete list_u2;
  cout << "List " << list.size() << " element(s) of original list:\n";
  list.show(NULL, payload_get);

  return 0;
}
