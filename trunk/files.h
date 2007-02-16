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

#ifndef FILES_H
#define FILES_H

typedef struct {
  mode_t  type;     // type
  time_t  mtime;    // time of last modification
  off_t   size;     // total size, in bytes
  uid_t   uid;      // user ID of owner
  gid_t   gid;      // group ID of owner
  mode_t  mode;     // permissions
} metadata_t;

typedef struct {
  string      path;
  char        checksum[36];
  metadata_t  metadata;
} filedata_t;

extern int metadata_get(const char *path, metadata_t *metadata);

/* Read parameters from line */
extern int params_readline(string line, char *keyword, char *type,
  string *);

class File {
  string  _path;      // file path
  string  _checksum;  // file checksum
  mode_t  _type;      // file type
  time_t  _mtime;     // time of last modification
  off_t   _size;      // total size, in bytes
  uid_t   _uid;       // user ID of owner
  gid_t   _gid;       // group ID of owner
  mode_t  _mode;      // permissions
public:
  // Test whether dir exists, create it if requested
  static int testDir(const string& path, bool create);
  // Test whether file exists, create it if requested
  static int testReg(const string& path, bool create);
  // Transform letter into mode
  static char typeLetter(mode_t mode);
  // Transform mode into letter
  static mode_t typeMode(char letter);
  // Convert MD5 to readable string
  static void md5sum(const char *checksum, int bytes);
  // Copy, compress and compute checksum (MD5), all in once
  static int zcopy(
    const string& source_path,
    const string& dest_path,
    off_t         *size_in,
    off_t         *size_out,
    char          *checksum_in,
    char          *checksum_out,
    int           compress);
  // Compute file checksum (MD5)
  static int getChecksum(const string& path, const char *checksum);
};

#endif
