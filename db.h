/*
     Copyright (C) 2006  Herve Fache

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

#ifndef COMMON_H
#error You must include common.h before db.h
#endif

typedef struct {
  string      host;
  filedata_t  filedata;
  string      link;
  time_t      date_in;
  time_t      date_out;
} db_data_t;

class Database {
  string _path;
  int  save(
    const string& filename,
    List *list);
  int  lock();
  void unlock();
public:
  Database(const string& path) : _path(path) {}
  /* Open database */
  int  open();
  /* Close database */
  void close();
  string mount() { return _path + "/mount"; }
  /* Check what needs to be done for given host & path */
  int  parse(
    const string& prefix,
    const string& real_path,
    const string& mount_path,
    List          *list);
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
  int  load(
    const string &filename,
    List *list);
  int  obsolete(
    const string& prefix,
    const string& path,
    const string& checksum);
  int  organize(
    const string& path,
    int number);
  int  write(
    const string&   mount_path,
    const string&   path,
    const db_data_t *db_data,
    char            *checksum,
    int             compress);
};

#endif
