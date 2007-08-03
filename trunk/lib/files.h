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

#ifndef FILES_H
#define FILES_H

#include <fstream>
#include <vector>

using namespace std;

#include <openssl/md5.h>
#include <openssl/evp.h>
#include <zlib.h>

namespace hbackup {

class GenericFile {
  char*     _path;      // file path
protected:
  char*     _name;      // file name
  char      _type;      // file type (0 if metadata not available)
  time_t    _mtime;     // time of last modification
  long long _size;      // on-disk size, in bytes
  uid_t     _uid;       // user ID of owner
  gid_t     _gid;       // group ID of owner
  mode_t    _mode;      // permissions
public:
  // Default constructor
  GenericFile(const GenericFile& g) :
        _path(NULL),
        _name(NULL),
        _type(g._type),
        _mtime(g._mtime),
        _size(g._size),
        _uid(g._uid),
        _gid(g._gid),
        _mode(g._mode) {
    asprintf(&_name, "%s", g._name);
  }
  // Constructor for path in the VFS
  GenericFile(const char *path, const char* name = "");
  // Constructor for given file metadata
  GenericFile(
      const char* name,
      char        type,
      time_t      mtime,
      long long   size,
      uid_t       uid,
      gid_t       gid,
      mode_t      mode) :
        _path(NULL),
        _name(NULL),
        _type(type),
        _mtime(mtime),
        _size(size),
        _uid(uid),
        _gid(gid),
        _mode(mode) {
    asprintf(&_name, "%s", name);
  }
  ~GenericFile() {
    free(_path);
    free(_name);
  }
  bool        isValid() const { return _type != '?'; }
  // Data read access
  const char* path()    const { return _path;  }
  const char* name()    const { return _name;  }
  char        type()    const { return _type;  }
  time_t      mtime()   const { return _mtime; }
  long long   size()    const { return _size;  }
  uid_t       uid()     const { return _uid;   }
  gid_t       gid()     const { return _gid;   }
  mode_t      mode()    const { return _mode;  }
};

class GenericFileListElement {
  GenericFile*            _payload;
  GenericFileListElement* _next;
public:
  GenericFileListElement(GenericFile* payload) :
    _payload(payload),
    _next(NULL) {}
  ~GenericFileListElement() {
    delete _payload;
    delete _next;
  }
  void insert(GenericFileListElement** first);
  GenericFile*            payload() { return _payload; }
  GenericFileListElement* next()    { return _next; }
};

class File2 : public GenericFile {
  char*     _checksum;
public:
  // Constructor for path in the VFS
  File2(const GenericFile& g) :
      GenericFile(g),
      _checksum(NULL) {}
  // Constructor for given file metadata
  File2(
    const char* name,
    char        type,
    time_t      mtime,
    long long   size,
    uid_t       uid,
    gid_t       gid,
    mode_t      mode,
    const char* checksum) :
        GenericFile(name, type, mtime, size, uid, gid, mode),
        _checksum(NULL) {
      asprintf(&_checksum, "%s", checksum);
  }
  ~File2() {
    free(_checksum);
  }
  // Data read access
  const char* checksum()  const { return _checksum;  }
};

class Directory : public GenericFile {
  GenericFileListElement* _first_entry;
  int entries;
  int read(const char* path);
public:
  // Constructor for path in the VFS
  Directory(const GenericFile& g, const char* path) :
      GenericFile(g),
      _first_entry(NULL),
      entries(0) {
    _size  = 0;
    _mtime = 0;
    // Create list of GenericFiles contained in directory
    if (read(path)) entries = -1;
  }
  ~Directory() {
    delete _first_entry;
  }
  // FIXME Temporary
  void showList() {
    GenericFileListElement* entry = _first_entry;
    while (entry != NULL) {
      cout << " -> name: " << entry->payload()->name() << endl;
      entry = entry->next();
    }
  }
};

class CharDev : public GenericFile {
};

class BlockDev : public GenericFile {
};

class Pipe : public GenericFile {
};

class Link : public GenericFile {
  char*     _link;
public:
  // Constructor for path in the VFS
  Link(const GenericFile& g, const char* path) :
      GenericFile(g),
      _link(NULL) {
    _mtime = 0;
    char* link = (char*) malloc(FILENAME_MAX + 1);
    int   size = readlink(path, link, FILENAME_MAX);

    if (size < 0) {
      _type = '?';
      free(link);
    } else {
      _link = (char*) malloc(size + 1);
      strncpy(_link, link, size);
      _link[size] = '\0';
    }
  }
  // Constructor for given file metadata
  Link(
    const char* name,
    char        type,
    time_t      mtime,
    long long   size,
    uid_t       uid,
    gid_t       gid,
    mode_t      mode,
    const char* link) :
        GenericFile(name, type, mtime, size, uid, gid, mode),
        _link(NULL) {
      asprintf(&_link, "%s", link);
  }
  ~Link() {
    free(_link);
  }
  // Data read access
  const char* link()  const { return _link;  }
};

class Socket : public GenericFile {
};

class File {
  string          _path;      // file path
  string          _checksum;  // file checksum
  string          _link;      // what the link points to (if a link, of course)
  mode_t          _type;      // file type (0 if metadata not available)
  time_t          _mtime;     // time of last modification
  long long       _size;      // on-disk size, in bytes
  uid_t           _uid;       // user ID of owner
  gid_t           _gid;       // group ID of owner
  mode_t          _mode;      // permissions
  FILE*           _fd;        // file descriptor
  long long       _dsize;     // uncompressed data size, in bytes
  bool            _fwrite;    // file open for write
  unsigned char*  _fbuffer;   // buffer for file compression during read/write
  bool            _fempty;    // compression buffer not empty
  bool            _feof;      // All data read AND decompressed
  EVP_MD_CTX*     _ctx;       // openssl resources
  z_stream*       _strm;      // zlib resources
  // Convert MD5 to readable string
  static void md5sum(
    string&               checksum_out,
    const unsigned char*  checksum_in,
    int                   bytes);
public:
  // Max buffer size for read/write
  static const size_t chunk = 409600;
  // Constructor for existing file (if only one argument, it will be the path)
  File(const string& access_path, const string& path = "");
  // Constructor for given file data
  File(
    const string& path,
    const string& link,
    mode_t        type,
    time_t        mtime,
    long long     size,
    uid_t         uid,
    gid_t         gid,
    mode_t        mode) :
      _path(path),
      _checksum(""),
      _link(link),
      _type(type),
      _mtime(mtime),
      _size(size),
      _uid(uid),
      _gid(gid),
      _mode(mode),
      _fd(NULL) {}
  // Constructor for given line
  File(char* line, size_t size);
  // Need '<' to sort list
  bool operator<(const File&) const;
  bool operator!=(const File&) const;
  bool operator==(const File& right) const { return ! (*this != right); }
  bool metadiffer(const File&) const;
  string name() const;
  string path() const { return _path; };
  string checksum() const { return _checksum; }
  string fullPath(int max_size = -1) const;
  mode_t type() const { return _type; }
  time_t mtime() const { return _mtime; };
  long long size() const { return _size; };
  long long dsize() const { return _dsize; };
  bool eof() const { return _feof; };
  // Line containing all data (argument for debug only)
  string line(bool nodates = false) const;
  void setPath(const string& path) { _path = path; }

  // Open file, for read or write (no append), with or without compression
  int open(const char* prepath, const char* req_mode,
    unsigned int compression = 0);
  // Close file, for read or write (no append), with or without compression
  int close();
  // Read file sets eof (check with eof()) when all data is read and ready
  ssize_t read(unsigned char* buffer, size_t count);
  // Write to file (signal end of file for compression end)
  ssize_t write(unsigned char* buffer, size_t count, bool eof);
  // Get a line of data
  ssize_t readLine(unsigned char* buffer, size_t count, char delim);
  // Add a line of data
  ssize_t writeLine(const unsigned char* buffer, size_t count, char delim);
  // Get parameters from one line of data
  ssize_t readParams(vector<string>& params);

  // Test whether dir exists, create it if requested
  static int testDir(const string& path, bool create);
  // Test whether file exists, create it if requested
  static int testReg(const string& path, bool create);
  // Transform letter into mode
  static char typeLetter(mode_t mode);
  // Transform mode into letter
  static mode_t typeMode(char letter);
  // Copy, compress and compute checksum (MD5), all in once
  static int zcopy(
    const string& source_path,
    const string& dest_path,
    long long*    size_in,
    long long*    size_out,
    string*       checksum_in,
    string*       checksum_out,
    int           compress);
  // Compute file checksum (MD5)
  static int getChecksum(const string& path, string& checksum);
  // Read parameters from line
  static int decodeLine(const string& line, vector<string>& params);
};

}

#endif
