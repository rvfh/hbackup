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

#ifndef DBDATA_H
#error You must include dbdata.h before db.h
#endif

#ifndef FILES_H
#error You must include files.h before db.h
#endif

#ifndef LIST_H
#error You must include list.h before db.h
#endif

namespace hbackup {

class Database {
  string              _path;
  SortedList<DbData>  _active;
  SortedList<DbData>  _removed;
  bool                _active_open;
  bool                _removed_open;
  int  lock();
  void unlock();
  int  save_journal(
    const string&       filename,
    SortedList<DbData>& list,
    unsigned int        offset = 0);
public:
  Database(const string& path) : _path(path), _active_open(false),
    _removed_open(false) {}
  /* Open database (calls open_active for now) */
  int  open();
  /* Open active records part of database */
  int  open_active();
  /* Open removed records part of database */
  int  open_removed();
  /* Close active records part of database */
  int  close_active();
  /* Close removed records part of database */
  int  close_removed();
  /* Close database */
  int  close();
  /* Check what needs to be done for given host & path */
  int  parse(
    const string& prefix,
    const string& real_path,
    const string& mount_path,
    list<File>*   list);
  // Expire data older then given time out (seconds)
  int expire(
    const string&   prefix,
    const string&   path,
    time_t          time_out);
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
// So I can test them/use them in tests
  int getDir(
    const string& checksum,
    string&       path,
    bool          create);
  // Load list, skipping offset elements
  int  load(
    const string&       filename,
    SortedList<DbData>& list,
    unsigned int        offset = 0);
  int  save(
    const string&       filename,
    SortedList<DbData>& list,
    bool                backup = false);
  int  organize(
    const string& path,
    int           number);
  int  write(
    const string&   path,
    DbData&         db_data,
    int             compress = 0);
// For debug only
  SortedList<DbData>* active() { return &_active; }
  SortedList<DbData>* removed() { return &_removed; }
};

}

#endif
