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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "strings.h"

using namespace hbackup;

String::String(const String& string) {
  _string = NULL;
  _length = asprintf(&_string, "%s", string._string);
  _size = _length + 1;
}

String::String(const char* string) {
  _string = NULL;
  _length = asprintf(&_string, "%s", string);
  _size = _length + 1;
}

String& String::operator=(const String& string) {
  if (string._length > _length) {
    _size   = string._length + 1;
    _length = _size -1;
    _string = (char*) realloc(_string, _size);
  }
  strcpy(_string, string._string);
  return *this;
}

String& String::operator=(const char* string) {
  int length = strlen(string);
  if (length > _length) {
    _size   = length + 1;
    _length = _size -1;
    _string = (char*) realloc(_string, _size);
  }
  strcpy(_string, string);
  return *this;
}

bool String::operator==(const String& string) const {
  return strcmp(_string, string._string) == 0;
}

bool String::operator==(const char* string) const {
  return strcmp(_string, string) == 0;
}

String& String::operator+(const String& string) {
  int length = _length + string._length;
  if (_size < length + 1) {
    _size = length + 1;
    _string = (char*) realloc(_string, _size);
  }
  strcpy(&_string[_length], string._string);
  _length = length;
  return *this;
}

String& String::operator+(const char* string) {
  int length = _length + strlen(string);
  if (_size < length + 1) {
    _size = length + 1;
    _string = (char*) realloc(_string, _size);
  }
  strcpy(&_string[_length], string);
  _length = length;
  return *this;
}
