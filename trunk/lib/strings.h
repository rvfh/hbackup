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

#ifndef STRINGS_H
#define STRINGS_H

namespace hbackup {

class String {
protected:
  char* _string;
  int   _length;
  int   _size;
public:
  String() :
    _string(NULL),
    _length(0),
    _size(0) {}
  String(const String& string);
  String(const char* string);
  ~String() {
    free(_string);
  }
  int length() const        { return _length; }
  const char* c_str() const { return _string; }
  String& operator=(const String& string);
  String& operator=(const char* string);
  bool    operator==(const String& string) const;
  bool    operator==(const char* string) const;
  bool    operator!=(const String& string) const {
    return ! (*this != string);
  }
  bool    operator!=(const char* string) const  {
    return ! (*this != string);
  }
  String& operator+(const String& string);
  String& operator+(const char* string);
};

}

#endif
