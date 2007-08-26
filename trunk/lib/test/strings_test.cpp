/*
     Copyright (C) 2006-2007  Herve Fache

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

#include <iostream>

using namespace std;

#include <stdio.h>
#include <stdlib.h>

#include "strings.h"

using namespace hbackup;

int main(void) {
  String str1("abc");
  cout << str1.length() << ": " << str1.c_str() << endl;

  String str2;
  const char* chr = "def";
  str2 = chr;
  cout << str2.length() << ": " << str2.c_str() << endl;
  if (str2.c_str() == chr) {
    cout << "Bug: copied the pointer!!!" << endl;
  }

  String str3;
  str3 = str1;
  cout << str3.length() << ": " << str3.c_str() << endl;
  if (str1.c_str() == str3.c_str()) {
    cout << "Bug: copied the pointer!!!" << endl;
  }

  String str4(str1);
  cout << str4.length() << ": " << str4.c_str() << endl;
  if (str1.c_str() == str4.c_str()) {
    cout << "Bug: copied the pointer!!!" << endl;
  }

  if (str1 == str2) {
    cout << str1.c_str() << " and " << str2.c_str() << " are equal" << endl;
  } else {
    cout << str1.c_str() << " and " << str2.c_str() << " differ" << endl;
  }

  if (str1 == str3) {
    cout << str1.c_str() << " and " << str3.c_str() << " are equal" << endl;
  } else {
    cout << str1.c_str() << " and " << str3.c_str() << " differ" << endl;
  }

  if (str1 == chr) {
    cout << str1.c_str() << " and " << chr << " are equal" << endl;
  } else {
    cout << str1.c_str() << " and " << chr << " differ" << endl;
  }

  if (str2 == chr) {
    cout << str2.c_str() << " and " << chr << " are equal" << endl;
  } else {
    cout << str2.c_str() << " and " << chr << " differ" << endl;
  }

  String str5 = str1 + str2;
  cout << str5.length() << ": " << str5.c_str() << endl;

  String str6 = str1 + "ghi";
  cout << str6.length() << ": " << str6.c_str() << endl;

  return 0;
}
