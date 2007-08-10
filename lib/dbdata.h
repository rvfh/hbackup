/*
     Copyright (C) 2007  Herve Fache

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

#ifndef DBDATA_H
#define DBDATA_H

#include "files.h"

namespace hbackup {

class DbData {
  File    _data;
  string  _prefix;
  string  _checksum;
  time_t  _in;
  time_t  _out;
  bool    _expired;
public:
  DbData(const File& data) :
    _data(data), _checksum(""), _out(0), _expired(false) { _in = time(NULL); }
  // line gets modified
  DbData(char* line, size_t size) : _data("", 0), _expired(false) {
    char* start  = line;
    char* value  = new char[size];
    int   failed = 0;

    for (int field = 1; field <= 12; field++) {
      // Get tabulation position
      char* delim = strchr(start, '\t');
      if (delim == NULL) {
        failed = 1;
      } else {
        // Get string portion
        strncpy(value, start, delim - start);
        value[delim - start] = '\0';
        /* Extract data */
        switch (field) {
          case 1:   /* Prefix */
            _prefix = value;
            break;
          case 2:   /* File data */
            _data = File(start, size - (start - line));
            break;
          case 10:  /* Checksum */
            _checksum = value;
            break;
          case 11:  /* Date in */
            if (sscanf(value, "%ld", &_in) != 1) {
              failed = 2;
            }
            break;
          case 12:  /* Date out */
            if (sscanf(value, "%ld", &_out) != 1) {
              failed = 2;
            }
        }
        start = delim + 1;
      }
      if (failed) {
        break;
      }
    }
    free(value);
    if ((failed != 0) || (_data.type() == 0)) {
      _in = 0;
    }
  }
  bool operator<(const DbData& right) const {
    int cmp = strcmp(_prefix.c_str(), right._prefix.c_str());
    if (cmp < 0)return true;
    else if (cmp > 0) return false;

    if (_data < right._data) return true;
    else if (right._data < _data) return false;

    // Equal then...
    return (_in < right._in)
      || ((_in == right._in) && (_checksum < right._checksum));
    // Note: checking for _out would break the journal replay stuff (uses find)
  }
  // Equality and difference exclude _out
  bool operator!=(const DbData& right) const {
    return (_prefix != right._prefix) || (_in != right._in)
        || (_checksum != right._checksum) || (_data != right._data);
  }
  bool   operator==(const DbData& right) const { return ! (*this != right); }
  File*  data() { return &_data; }
  string prefix() const { return _prefix; }
  string checksum() const { return _checksum; }
  time_t in() const { return _in; }
  time_t out() const { return _out; }
  bool   expired() { return _expired; }
  void   setPrefix(const string& prefix) { _prefix = prefix; }
  void   setChecksum(const string& checksum) { _checksum = checksum; }
  void   setOut() { _out = time(NULL); }
  void   setOut(time_t out) { _out = out; }
  void   resetExpired() { _expired = false; }
  void   setExpired() { _expired = true; }
  string fullPath(int size_max) const {
    // Simple and inefficient
    string full_path = _prefix + "/" + _data.path();
    if (size_max < 0) {
      return full_path;
    }
    return full_path.substr(0, size_max);
  }
  string line(bool nodates = false) const {
    char*   numbers = NULL;
    string  output = _prefix + "\t" + _data.path();

    asprintf(&numbers, "%c\t%lld\t%ld\t%u\t%u\t%o", _data.type(),
      _data.size(), nodates ? (_data.mtime() != 0) : _data.mtime(),
      _data.uid(), _data.gid(), _data.mode());
    output += "\t" + string(numbers) + "\t" + _data.link() + "\t" + _checksum;
    delete numbers;

    asprintf(&numbers, "%ld\t%ld", _in, _out);
    output += "\t" + string(numbers) + "\t";
    delete numbers;
    return output;
  }
};

}

#endif
