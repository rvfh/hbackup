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

/* Open database */
extern int db_open(const string& db_path);

/* Close database */
extern void db_close(void);

/* Check what needs to be done for given prefix (protocol://host/share) */
extern int db_parse(
  const string& prefix,
  const string& real_path,
  const string& mount_path,
  List *list);

/* Read file with given checksum, extract it to path */
extern int db_read(const string& path, const string& checksum);

/* Check database for missing/corrupted data */
/* If local_db_path is empty, use already open database */
/* If checksum is empty, scan all contents */
/* TODO If thorough is true, check for corruption */
extern int db_scan(
  const string& checksum = "",
  bool thorough = false);

#endif
