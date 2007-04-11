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

#ifndef DB_H
#define DB_H

#ifndef LIST_H
#error You must include list.h before db.h
#endif

#ifndef FILES_H
#error You must include files.h before db.h
#endif

class DbData {
  time_t  _in;
  time_t  _out;
  string  _checksum;
  File    _data;
public:
  DbData(const File& data) : _out(0), _checksum(""), _data(data) {
    _in = time(NULL);
  }
  DbData(time_t in, time_t out, string checksum, const File& data) :
    _in(in), _out(out), _checksum(checksum), _data(data) {}
  bool operator<(const DbData&) const;
  bool operator!=(const DbData&) const;
  bool operator==(const DbData& right) const { return ! (*this != right); }
  time_t in() const { return _in; }
  time_t out() const { return _out; }
  string checksum() const { return _checksum; }
  File   data() const { return _data; }
  void   setOut() { _out = time(NULL); }
  void   setChecksum(const string& checksum) { _checksum = checksum; }
  string line(bool nodates = false) const;
};

class Database {
  string             _path;
  SortedList<DbData> _active;
  SortedList<DbData> _removed;
  int  lock();
  void unlock();
  int  save(
    const string&       filename,
    SortedList<DbData>& list,
    bool                backup = false);
  int  save_journal(
    const string&                         filename,
    vector<SortedList<DbData>::iterator>& vector);
  // Recover file lists
  int  recover();
public:
  Database(const string& path) : _path(path) {}
  /* Open database */
  int  open();
  /* Close database */
  int  close();
  /* Check what needs to be done for given host & path */
  int  parse(
    const string& prefix,
    const string& real_path,
    const string& mount_path,
    list<File>*   list);
  /* Read file with given checksum, extract it to path */
  int  read(
    const string& path,
    const string& checksum);
  /* Check database for missing/corrupted data */
  /* If local_db_path is empty, use already open database */
  /* If checksum is empty, scan all contents */
  /* If thorough is true, check for corruption */
  int  scan(
    const string& checksum = "",
    bool thorough = false);
// So I can test them
  int getDir(
    const string& checksum,
    string&       path,
    bool          create);
  int  load(
    const string &filename,
    SortedList<DbData>& list);
  int  organize(
    const string& path,
    int number);
  int  write(
    const string&   path,
    DbData&         db_data,
    int             compress = 0);
// For debug only
  SortedList<DbData>* active() { return &_active; }
  SortedList<DbData>* removed() { return &_removed; }
};

#endif
