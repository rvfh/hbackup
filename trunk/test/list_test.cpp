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
#include <string>
#include <list>

#include "list.h"

typedef struct {
  char name[256];
} payload_t;

typedef struct {
  char dir[256];
  char filename[256];
} payload2_t;

static string payload_get(const void *payload_p) {
  const payload_t *payload = (const payload_t *) (payload_p);
  return payload->name;
}

static string payload2_get(const void *payload_p) {
  const payload2_t *payload = (const payload2_t *) (payload_p);
  return string(payload->dir) + "/" + string(payload->filename);
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
  SortedList<string> sorted_list;
  list<string>::iterator i;

  cout << "Fill in list\n";
  i = sorted_list.find("test");
  sorted_list.add("test/testfile");
  cout << "List " << sorted_list.size() << " element(s):\n";
  for (i = sorted_list.begin(); i != sorted_list.end(); i++) {
    cout << *i << endl;
  }
  i = sorted_list.find("test");

  sorted_list.add("test");
  cout << "List " << sorted_list.size() << " element(s):\n";
  for (i = sorted_list.begin(); i != sorted_list.end(); i++) {
    cout << *i << endl;
  }

  i = sorted_list.find("test");
  if (*i != "test") {
    cout << "test not found???" << endl;
  }

  sorted_list.add("test/subdir/testfile1");
  cout << "List " << sorted_list.size() << " element(s):\n";
  for (i = sorted_list.begin(); i != sorted_list.end(); i++) {
    cout << *i << endl;
  }

  sorted_list.add("test/subdir");
  cout << "List " << sorted_list.size() << " element(s):\n";
  for (i = sorted_list.begin(); i != sorted_list.end(); i++) {
    cout << *i << endl;
  }

  sorted_list.add("test/testfile");
  cout << "List " << sorted_list.size() << " element(s):\n";
  for (i = sorted_list.begin(); i != sorted_list.end(); i++) {
    cout << *i << endl;
  }

  cout << "Empty list\n";
  sorted_list.erase(sorted_list.begin());
  cout << "List " << sorted_list.size() << " element(s):\n";
  for (i = sorted_list.begin(); i != sorted_list.end(); i++) {
    cout << *i << endl;
  }

  i-- = sorted_list.end();
  sorted_list.erase(i);
  cout << "List " << sorted_list.size() << " element(s):\n";
  for (i = sorted_list.begin(); i != sorted_list.end(); i++) {
    cout << *i << endl;
  }

  i--;
  i--;
  sorted_list.erase(i);
  cout << "List " << sorted_list.size() << " element(s):\n";
  for (i = sorted_list.begin(); i != sorted_list.end(); i++) {
    cout << *i << endl;
  }

  sorted_list.erase(sorted_list.begin());
  cout << "List " << sorted_list.size() << " element(s):\n";
  for (i = sorted_list.begin(); i != sorted_list.end(); i++) {
    cout << *i << endl;
  }

  i-- = sorted_list.end();
  sorted_list.erase(i);
  cout << "List " << sorted_list.size() << " element(s):\n";
  for (i = sorted_list.begin(); i != sorted_list.end(); i++) {
    cout << *i << endl;
  }



  list_entry_t  *entry              = NULL;
  payload_t     payload;
  payload2_t    payload2;
  List          list(payload_get);
  List          *list2 = new List(payload2_get);
  List          list3(payload2_get);

  cout << "\nFill in list\n";
  strcpy(payload.name, "test");
  list.add(list_test_new(payload));
  cout << "List " << list.size() << " element(s):\n";
  list.show(NULL, payload_get);

  strcpy(payload.name, "test/testfile");
  list.add(list_test_new(payload));
  cout << "List " << list.size() << " element(s):\n";
  list.show(NULL, payload_get);
  if (list.find("test", NULL, &entry)) {
    cout << "test not found???\n";
  }

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

  return 0;
}
