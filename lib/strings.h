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
  char*   _string;
  int     _length;
  size_t  _size;
  void alloc(size_t size);
public:
  String();
  String(const String& string);
  String(const char* string, int length = -1);
  virtual ~String() {
    free(_string);
  }
  int length() const        { return _length; }
  const char* c_str() const { return _string; }
  String& operator=(const char* string);
  String& operator=(const String& string);
  virtual int compare(const char* string) const;
  virtual int compare(const String& string) const {
    return compare(string._string);
  }
  virtual bool operator<(const char* string) const {
    return compare(string) < 0;
  }
  virtual bool operator<(const String& string) const {
    return compare(string) < 0;
  }
  virtual bool operator>(const char* string) const {
    return compare(string) > 0;
  }
  virtual bool operator>(const String& string) const {
    return compare(string) > 0;
  }
  virtual bool operator<=(const char* string) const {
    return compare(string) <= 0;
  }
  virtual bool operator<=(const String& string) const {
    return compare(string) <= 0;
  }
  virtual bool operator>=(const char* string) const {
    return compare(string) >= 0;
  }
  virtual bool operator>=(const String& string) const {
    return compare(string) >= 0;
  }
  virtual bool operator==(const char* string) const {
    return compare(string) == 0;
  }
  virtual bool operator==(const String& string) const {
    return compare(string) == 0;
  }
  virtual bool operator!=(const char* string) const {
    return compare(string) != 0;
  }
  virtual bool operator!=(const String& string) const {
    return compare(string) != 0;
  }
  String& operator+(const char* string);
  String& operator+(const String& string);
};

class StrPath : public String {
public:
  using String::operator=;
  StrPath() :
    String::String() {}
  StrPath(const char* string, int length = -1) :
    String::String(string, length) {}
  StrPath(const String& string) :
    String::String(string) {}
  int compare(const char* string, size_t length = -1) const;
  int compare(const String& string, size_t length = -1) const {
    return compare(string.c_str(), length);
  }
  int compare(const StrPath& string, size_t length = -1) const {
    return compare(string._string, length);
  }
  bool    operator<(const char* string) const {
    return compare(string) < 0;
  }
  bool    operator>(const char* string) const {
    return compare(string) > 0;
  }
  bool    operator<=(const char* string) const {
    return compare(string) <= 0;
  }
  bool    operator>=(const char* string) const {
    return compare(string) >= 0;
  }
  bool    operator==(const char* string) const {
    return compare(string) == 0;
  }
  bool    operator!=(const char* string) const {
    return compare(string) != 0;
  }
  bool    operator<(const String& string) const {
    return compare(string) < 0;
  }
  bool    operator>(const String& string) const {
    return compare(string) > 0;
  }
  bool    operator<=(const String& string) const {
    return compare(string) <= 0;
  }
  bool    operator>=(const String& string) const {
    return compare(string) >= 0;
  }
  bool    operator==(const String& string) const {
    return compare(string) == 0;
  }
  bool    operator!=(const String& string) const {
    return compare(string) != 0;
  }
  bool    operator<(const StrPath& string) const {
    return compare(string) < 0;
  }
  bool    operator>(const StrPath& string) const {
    return compare(string) > 0;
  }
  bool    operator<=(const StrPath& string) const {
    return compare(string) <= 0;
  }
  bool    operator>=(const StrPath& string) const {
    return compare(string) >= 0;
  }
  bool    operator==(const StrPath& string) const {
    return compare(string) == 0;
  }
  bool    operator!=(const StrPath& string) const {
    return compare(string) != 0;
  }
  const StrPath& toUnix();
  const StrPath& noEndingSlash();
  const StrPath  basename();
  const StrPath  dirname();
};

}

#endif
