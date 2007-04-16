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

  return 0;
}
