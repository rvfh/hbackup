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
  cout << endl << "1:" << endl;
  String str1("abcdefghi", 3);
  cout << str1.length() << ": " << str1.c_str() << endl;
  str1 = "1234567890123456789012345678901234567890123456789012345678901234567890";
  cout << str1.length() << ": " << str1.c_str() << endl;
  str1 = "abc";
  cout << str1.length() << ": " << str1.c_str() << endl;

  cout << endl << "2:" << endl;
  String str2;
  const char* chr = "def";
  str2 = chr;
  cout << str2.length() << ": " << str2.c_str() << endl;
  if (str2.c_str() == chr) {
    cout << "Bug: copied the pointer!!!" << endl;
  }

  cout << endl << "3:" << endl;
  String str3;
  str3 = str1;
  cout << str3.length() << ": " << str3.c_str() << endl;
  if (str1.c_str() == str3.c_str()) {
    cout << "Bug: copied the pointer!!!" << endl;
  }

  cout << endl << "4:" << endl;
  String str4(str1);
  cout << str4.length() << ": " << str4.c_str() << endl;
  if (str1.c_str() == str4.c_str()) {
    cout << "Bug: copied the pointer!!!" << endl;
  }

  cout << endl << "5:" << endl;
  switch (str1.compare(str2)) {
    case 1:
      cout << str1.c_str() << " > " << str2.c_str() << endl;
      break;
    case -1:
      cout << str1.c_str() << " < " << str2.c_str() << endl;
      break;
    default:
      cout << str1.c_str() << " == " << str2.c_str() << endl;
  }

  switch (str1.compare(str3)) {
    case 1:
      cout << str1.c_str() << " > " << str3.c_str() << endl;
      break;
    case -1:
      cout << str1.c_str() << " < " << str3.c_str() << endl;
      break;
    default:
      cout << str1.c_str() << " == " << str3.c_str() << endl;
  }

  switch (str2.compare(str1)) {
    case 1:
      cout << str2.c_str() << " > " << str1.c_str() << endl;
      break;
    case -1:
      cout << str2.c_str() << " < " << str1.c_str() << endl;
      break;
    default:
      cout << str2.c_str() << " == " << str1.c_str() << endl;
  }

  switch (str1.compare(chr)) {
    case 1:
      cout << str1.c_str() << " > " << chr << endl;
      break;
    case -1:
      cout << str1.c_str() << " < " << chr << endl;
      break;
    default:
      cout << str1.c_str() << " == " << chr << endl;
  }

  switch (str2.compare(chr)) {
    case 1:
      cout << str2.c_str() << " > " << chr << endl;
      break;
    case -1:
      cout << str2.c_str() << " < " << chr << endl;
      break;
    default:
      cout << str2.c_str() << " == " << chr << endl;
  }

  cout << endl << "6:" << endl;
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

  cout << endl << "7:" << endl;
  String str5 = str1 + str2;
  cout << str5.length() << ": " << str5.c_str() << endl;

  String str6 = str1 + "ghi";
  cout << str6.length() << ": " << str6.c_str() << endl;

  cout << endl << "Test: pathCompare" << endl;
  cout << "a <> a: " << StrPath("a").compare("a") << endl;
  cout << "a <> b: " << StrPath("a").compare("b") << endl;
  cout << "b <> a: " << StrPath("b").compare("a") << endl;
  cout << "a1 <> b: " << StrPath("a1").compare("b") << endl;
  cout << "b <> a1: " << StrPath("b").compare("a1") << endl;
  cout << "a1 <> a: " << StrPath("a1").compare("a") << endl;
  cout << "a <> a1: " << StrPath("a").compare("a1") << endl;
  cout << "a/ <> a: " << StrPath("a/").compare("a") << endl;
  cout << "a <> a/: " << StrPath("a").compare("a/") << endl;
  cout << "a\t <> a/: " << StrPath("a\t").compare("a/") << endl;
  cout << "a/ <> a\t " << StrPath("a/").compare("a\t") << endl;
  cout << "a\t <> a\t " << StrPath("a\t").compare("a\t") << endl;
  cout << "a\n <> a/: " << StrPath("a\n").compare("a/") << endl;
  cout << "a/ <> a\n " << StrPath("a/").compare("a\n") << endl;
  cout << "a\n <> a\n " << StrPath("a\n").compare("a\n") << endl;
  cout << "a/ <> a.: " << StrPath("a/").compare("a.") << endl;
  cout << "a. <> a/: " << StrPath("a.").compare("a/") << endl;
  cout << "a/ <> a-: " << StrPath("a/").compare("a-") << endl;
  cout << "a- <> a/: " << StrPath("a-").compare("a/") << endl;
  cout << "a/ <> a/: " << StrPath("a/").compare("a/") << endl;
  cout << "abcd <> abce, 3: " << StrPath("abcd").compare("abce", 3) << endl;
  cout << "abcd <> abce, 4: " << StrPath("abcd").compare("abce", 4) << endl;
  cout << "abcd <> abce, 5: " << StrPath("abcd").compare("abce", 5) << endl;

  cout << endl << "Test: toUnix" << endl;
  StrPath pth1 = "this\\is a path/ Unix\\ cannot cope with/\\";
  cout << pth1.c_str() << " -> ";
  cout << pth1.toUnix().c_str() << endl;
  cout << pth1.c_str() << " -> ";
  cout << pth1.toUnix().c_str() << endl;

  cout << endl << "Test: noEndingSlash" << endl;
  cout << pth1.c_str() << " -> ";
  cout << pth1.noEndingSlash().c_str() << endl;
  cout << pth1.c_str() << " -> ";
  cout << pth1.noEndingSlash().c_str() << endl;

  cout << endl << "Test: basename and dirname" << endl;
  pth1 = "this/is a path/to a/file";
  cout << pth1.c_str();
  cout << " -> base: ";
  cout << pth1.basename().c_str();
  cout << ", dir: ";
  cout << pth1.dirname().c_str();
  cout << endl;
  pth1 = "this is a file";
  cout << pth1.c_str();
  cout << " -> base: ";
  cout << pth1.basename().c_str();
  cout << ", dir: ";
  cout << pth1.dirname().c_str();
  cout << endl;
  pth1 = "this is a path/";
  cout << pth1.c_str();
  cout << " -> base: ";
  cout << pth1.basename().c_str();
  cout << ", dir: ";
  cout << pth1.dirname().c_str();
  cout << endl;

  cout << endl << "Test: comparators" << endl;
  pth1 = "this is a path/";
  StrPath pth2 = "this is a path.";
  cout << pth1.c_str() << " == " << pth2.c_str() << ": " << int(pth1 == pth2)
    << endl;
  cout << pth1.c_str() << " != " << pth2.c_str() << ": " << int(pth1 != pth2)
    << endl;
  cout << pth1.c_str() << " < " << pth2.c_str() << ": " << int(pth1 < pth2)
    << endl;
  cout << pth1.c_str() << " > " << pth2.c_str() << ": " << int(pth1 > pth2)
    << endl;
  str2 = "this is a path#";
  cout << pth1.c_str() << " < " << str2.c_str() << ": " << int(pth1 < str2)
    << endl;
  cout << pth1.c_str() << " > " << str2.c_str() << ": " << int(pth1 > str2)
    << endl;
  cout << pth1.c_str() << " < " << "this is a path-" << ": "
    << int(pth1 < "this is a path-") << endl;
  cout << pth1.c_str() << " > " << "this is a path-" << ": "
    << int(pth1 > "this is a path-") << endl;
  pth2 = "this is a path to somewhere";
  cout << pth1.c_str() << " == " << pth2.c_str() << ": " << int(pth1 == pth2)
    << endl;
  cout << pth1.c_str() << " != " << pth2.c_str() << ": " << int(pth1 != pth2)
    << endl;
  cout << pth1.c_str() << " <= " << pth2.c_str() << ": " << int(pth1 <= pth2)
    << endl;
  cout << pth1.c_str() << " >= " << pth2.c_str() << ": " << int(pth1 >= pth2)
    << endl;

  return 0;
}
