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
public:
  DbData(const File& data) :
    _data(data), _checksum(""), _out(0) { _in = time(NULL); }
  DbData(time_t in, time_t out, string checksum, const File& data) :
    _data(data), _checksum(checksum), _in(in), _out(out) {}
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
  bool operator==(const DbData& right) const { return ! (*this != right); }
  File   data() const { return _data; }
  string checksum() const { return _checksum; }
  time_t in() const { return _in; }
  time_t out() const { return _out; }
  void   setChecksum(const string& checksum) { _checksum = checksum; }
  void   setOut(time_t out = 0) {
    if (out != 0) {
      _out = out;
    } else {
      _out = time(NULL);
    }
  }
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
