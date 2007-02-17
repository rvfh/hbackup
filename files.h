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
  string      checksum;
  metadata_t  metadata;
} filedata_t;

extern int metadata_get(const char *path, metadata_t *metadata);

/* Read parameters from line */
extern int params_readline(string line, char *keyword, char *type,
  string *);

class File {
  string  _access_path; // mount or share path or prrefix
  string  _path;        // file path
  string  _checksum;    // file checksum
  mode_t  _type;        // file type (0 if metadata not available)
  time_t  _mtime;       // time of last modification
  off_t   _size;        // total size, in bytes
  uid_t   _uid;         // user ID of owner
  gid_t   _gid;         // group ID of owner
  mode_t  _mode;        // permissions
public:
  // Constructor for existing file
  File(const string& access_path, const string& path);
  // Constructor for given file data
  File(
    const string& access_path,
    const string& path,
    const string& checksum,
    mode_t        type,
    time_t        mtime,
    off_t         size,
    uid_t         uid,
    gid_t         gid,
    mode_t        mode) :
      _access_path(access_path),
      _path(path),
      _checksum(checksum),
      _type(type),
      _mtime(mtime),
      _size(size),
      _uid(uid),
      _gid(gid),
      _mode(mode) {}
  string name() const;
//   string access_path() const { return _access_path; };
  string path() const { return _path; };
  string checksum() const { return _checksum; };
  mode_t type() const { return _type; }
  time_t mtime() const { return _mtime; };
  off_t  size() const { return _size; };
//   uid_t  uid() const { return _uid; };
//   gid_t  gid() const { return _gid; };
//   mode_t mode() const { return _mode; };
  // Transitional function
  metadata_t metadata() const;
  void setChecksum(const string& checksum) { _checksum = checksum; }
// These are public
  // Test whether dir exists, create it if requested
  static int testDir(const string& path, bool create);
  // Test whether file exists, create it if requested
  static int testReg(const string& path, bool create);
  // Transform letter into mode
  static char typeLetter(mode_t mode);
  // Transform mode into letter
  static mode_t typeMode(char letter);
  // Convert MD5 to readable string
  static void md5sum(
    string&               checksum_out,
    const unsigned char*  checksum_in,
    int                   bytes);
  // Copy, compress and compute checksum (MD5), all in once
  static int zcopy(
    const string& source_path,
    const string& dest_path,
    off_t*        size_in,
    off_t*        size_out,
    string*       checksum_in,
    string*       checksum_out,
    int           compress);
  // Compute file checksum (MD5)
  static int getChecksum(const string& path, string& checksum);
};

#endif
