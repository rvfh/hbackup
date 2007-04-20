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

#ifndef DBLIST_H
#error You must include dblist.h before db.h
#endif

#ifndef FILES_H
#error You must include files.h before db.h
#endif

namespace hbackup {

class Database {
  string  _path;
  DbList  _active;
  DbList  _removed;
  int  lock();
  void unlock();
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
  int expire_init() {
    return _removed.open(_path, "removed");
  }
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
  int  organize(
    const string& path,
    int           number);
  int  write(
    const string&   path,
    DbData&         db_data,
    int             compress = 0);
// For debug only
  DbList* active() { return &_active; }
  DbList* removed() { return &_removed; }
};

}

#endif
