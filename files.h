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

class File {
  string  _prefix;      // mount or share path or prrefix
  string  _path;        // file path
  string  _link;        // what the link points to (if a link, of course)
  mode_t  _type;        // file type (0 if metadata not available)
  time_t  _mtime;       // time of last modification
  off_t   _size;        // total size, in bytes
  uid_t   _uid;         // user ID of owner
  gid_t   _gid;         // group ID of owner
  mode_t  _mode;        // permissions
public:
  // Constructor for existing file (if only one argument, it will be the path)
  File(const string& access_path, const string& path = "");
  // Constructor for given file data
  File(
    const string& prefix,
    const string& path,
    const string& link,
    mode_t        type,
    time_t        mtime,
    off_t         size,
    uid_t         uid,
    gid_t         gid,
    mode_t        mode) :
      _prefix(prefix),
      _path(path),
      _link(link),
      _type(type),
      _mtime(mtime),
      _size(size),
      _uid(uid),
      _gid(gid),
      _mode(mode) {}
  // Need '<' to sort list
  bool operator<(const File&) const;
  bool operator!=(const File&) const;
  bool operator==(const File& right) const { return ! (*this != right); }
  bool metadiffer(const File&) const;
  string name() const;
  string prefix() const { return _prefix; };
  string path() const { return _path; };
  mode_t type() const { return _type; }
  time_t mtime() const { return _mtime; };
  off_t  size() const { return _size; };
  // Line containing all data (argument for debug only)
  string line(bool nodates = false) const;
  void setPrefix(const string& prefix) { _prefix = prefix; }
  void setPath(const string& path) { _path = path; }
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
  // Read parameters from line
  static int decodeLine(const string& line, vector<string>& params);
};

#endif
