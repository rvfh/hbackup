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

void String::alloc(size_t size) {
  size >>= 5;
  size++;
  size <<= 5;
  if (size > _size) {
    _size = size;
    _string = (char*) realloc(_string, _size);
  }
}

String::String() {
  _length = 0;
  _size   = 0;
  _string = NULL;
  alloc(_length + 1);
  _string[0] = '\0';
}

String::String(const String& string) {
  _length = string._length;
  _size   = 0;
  _string = NULL;
  alloc(_length + 1);
  strcpy(_string, string._string);
}

String::String(const char* string, int length) {
  if (length >= 0) {
    _length = length;
  } else {
    _length = strlen(string);
  }
  _size   = 0;
  _string = NULL;
  alloc(_length + 1);
  strncpy(_string, string, _length);
  _string[_length] = '\0';
  _length = strlen(_string);
}

String& String::operator=(const String& string) {
  _length = string._length;
  alloc(_length + 1);
  strcpy(_string, string._string);
  return *this;
}

String& String::operator=(const char* string) {
  _length = strlen(string);
  alloc(_length + 1);
  strcpy(_string, string);
  return *this;
}

String String::operator+(const char* string) {
  String lstring = *this;
  lstring += string;
  return lstring;
}

String String::operator+(const String& string) {
  String lstring = *this;
  lstring += string;
  return lstring;
}

String& String::operator+=(const char* string) {
  size_t length = _length + strlen(string);
  alloc(length + 1);
  strcpy(&_string[_length], string);
  _length = length;
  return *this;
}

String& String::operator+=(const String& string) {
  size_t length = _length + string._length;
  alloc(length + 1);
  strcpy(&_string[_length], string._string);
  _length = length;
  return *this;
}

int String::compare(const char* string) const {
  return strcmp(_string, string);
}

StrPath::StrPath(const char* dir, const char* name) {
  _length = strlen(dir) + strlen(name);
  // Leave space for the '/'
  alloc(_length + 2);
  if (dir[0] == '\0') {
    strcpy(_string, name);
  } else if (name[0] == '\0') {
    strcpy(_string, dir);
  } else {
    sprintf(_string, "%s/%s", dir, name);
  }
}

int StrPath::compare(const char* string, size_t length) const {
  const char* s1 = _string;
  const char* s2 = string;
  while (true) {
    if (length == 0) {
      return 0;
    }
    if (*s1 == '\0') {
      if (*s2 == '\0') {
        return 0;
      } else {
        return -1;
      }
    } else {
      if (*s2 == '\0') {
        return 1;
      } else {
        if (*s1 == '/') {
          if (*s2 == '/') {
            s1++;
            s2++;
          } else {
            if (*s2 < ' ') {
              return 1;
            } else {
              return -1;
            }
          }
        } else {
          if (*s2 == '/') {
            if (*s1 < ' ') {
              return -1;
            } else {
              return 1;
            }
          } else {
            if (*s1 < *s2) {
              return -1;
            } else if (*s1 > *s2) {
              return 1;
            } else {
              s1++;
              s2++;
            }
          }
        }
      }
    }
    if (length > 0) {
      length--;
    }
  }
}

const StrPath& StrPath::toUnix() {
  char* pos = &_string[_length];
  while (--pos >= _string) {
    if (*pos == '\\') {
      *pos = '/';
    }
  }
  return *this;
}

const StrPath& StrPath::noEndingSlash() {
  char* pos = &_string[_length];
  while ((--pos >= _string) && (*pos == '/')) {
    *pos = '\0';
    _length--;
  }
  return *this;
}

const StrPath StrPath::basename() {
  const char* pos = strrchr(_string, '/');
  if (pos == NULL) {
    return StrPath(*this);
  } else {
    return StrPath(++pos);
  }
}

const StrPath StrPath::dirname() {
  const char* pos = strrchr(_string, '/');
  if (pos != NULL) {
    return StrPath(_string, pos - _string);
  } else {
    return StrPath(".");
  }
}
