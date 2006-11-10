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

/* Open database */
extern int db_open(const char *db_path);

/* Close database */
extern void db_close(void);

/* Check what needs to be done for given prefix (protocol://host/share) */
extern int db_parse(const char *prefix, const char *real_path,
  const char *mount_path, list_t list, size_t compress_min);

/* Read file with given checksum, extract it to path */
extern int db_read(const char *path, const char *checksum);

/* Check that data for given checksum exists (if NULL, scan all) */
extern int db_scan(const char *db_path, const char *checksum);

/* Check that data for given checksum is not corrupted (if NULL, check all) */
extern int db_check(const char *db_path, const char *checksum);

#endif
