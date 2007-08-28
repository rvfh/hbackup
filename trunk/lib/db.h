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

#include "strings.h"
// TODO remove this one by moving parsing into paths.cpp
#include "files.h"
// TODO remove this one by doing the data writing part in paths.cpp
#include "dbdata.h"

namespace hbackup {

class Database {
  struct        Private;
  Private*      _d;
  string        _path;
  list<string>  _active_checksums;
  bool          _expire_inited;
  int  lock();
  void unlock();
public:
  Database(const string& path);
  ~Database();
  string path() const { return _path; }
  /* Open database */
  int  open();
  /* Close database */
  int  close();
  // Prepare list for parser
  void getList(
    const char*     prefix,
    const char*     base_path,
    const char*     rel_path,
    list<Node*>&    list);
  /* Read file with given checksum, extract it to path */
  int  read(
    const string&   path,
    const string&   checksum);
  /* Check database for missing/corrupted data */
  /* If local_db_path is empty, use already open database */
  /* If checksum is empty, scan all contents */
  /* If thorough is true, check for corruption */
  int  scan(
    const String&   checksum = "",
    bool            thorough = false);
// So I can test them/use them in tests
  int getDir(
    const string&   checksum,
    string&         path,
    bool            create);
  int  organise(
    const string&   path,
    int             number);
  int  write(
    const string&   path,
    char**          checksum,
    int             compress = 0);
  int add(
    const char*     prefix,           // Client
    const char*     base_path,        // Path being backed up
    const char*     rel_path,         // Dir (from base_path)
    const char*     dir_path,         // Local dir below file
    const Node*     node,             // File
    const char*     checksum = NULL); // Do not copy data, use given checksum
  void remove(                    // Should not fail
    const char*     prefix,           // Client
    const char*     base_path,        // Path being backed up
    const char*     rel_path,         // Dir (from base_path)
    const Node*     node);            // File
// For debug only
  void* active();
};

}

#endif
