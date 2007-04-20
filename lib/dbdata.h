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

#ifndef FILES_H
#error You must include files.h before dbdata.h
#endif

namespace hbackup {

class DbData {
  File    _data;
  string  _checksum;
  time_t  _in;
  time_t  _out;
  bool    _expired;
public:
  DbData(const File& data) :
    _data(data), _checksum(""), _out(0), _expired(false) { _in = time(NULL); }
  // line gets modified
  DbData(char* line, size_t size) : _data(line, size), _expired(false) {
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
    if (_data < right._data) return true;
    if (right._data < _data) return false;
    // Equal then...
    return (_in < right._in)
      || ((_in == right._in) && (_checksum < right._checksum));
  // Note: checking for _out would break the journal replay stuff (uses find)
  }
  // Equality and difference exclude _out
  bool operator!=(const DbData& right) const {
    return (_in != right._in) || (_checksum != right._checksum)
    || (_data != right._data);
  }
  bool   operator==(const DbData& right) const { return ! (*this != right); }
  File   data() const { return _data; }
  string checksum() const { return _checksum; }
  time_t in() const { return _in; }
  time_t out() const { return _out; }
  bool   expired() { return _expired; }
  void   setChecksum(const string& checksum) { _checksum = checksum; }
  void   setOut() { _out = time(NULL); }
  void   setOut(time_t out) { _out = out; }
  void   resetExpired() { _expired = false; }
  void   setExpired() { _expired = true; }
  string line(bool nodates = false) const {
    string  output = _data.line(nodates) + "\t" + _checksum;
    char*   numbers = NULL;

    asprintf(&numbers, "%ld\t%ld", _in, _out);
    output += "\t" + string(numbers) + "\t";
    delete numbers;
    return output;
  }
};

}

#endif
