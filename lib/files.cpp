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

#include <iostream>
#include <vector>
#include <string>
#include <cctype>
#include <cstdio>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

/* I want to use the C file functions */
#undef open
#undef close
#undef read
#undef write
namespace std {
  using ::open;
  using ::close;
  using ::read;
  using ::write;
}

using namespace std;

#include "files.h"
#include "hbackup.h"

using namespace hbackup;

void Node::metadata(const char* path) {
  struct stat64 metadata;
  if (lstat64(path, &metadata)) {
    // errno set by lstat
    _type = '?';
  } else {
    if (S_ISREG(metadata.st_mode))       _type =  'f';
    else if (S_ISDIR(metadata.st_mode))  _type =  'd';
    else if (S_ISCHR(metadata.st_mode))  _type =  'c';
    else if (S_ISBLK(metadata.st_mode))  _type =  'b';
    else if (S_ISFIFO(metadata.st_mode)) _type =  'p';
    else if (S_ISLNK(metadata.st_mode))  _type =  'l';
    else if (S_ISSOCK(metadata.st_mode)) _type =  's';
    else                                 _type =  '?';
    /* Fill in file information */
    _size  = metadata.st_size;
    _mtime = metadata.st_mtime;
    _uid   = metadata.st_uid;
    _gid   = metadata.st_gid;
    _mode  = metadata.st_mode & ~S_IFMT;
  }
}

Node::Node(const char* path, const char* name) {
  _parsed = false;
  if (name[0] == '\0') {
    const char* name = strrchr(path, '/');

    _path = NULL;
    asprintf(&_path, "%s", path);

    _name = NULL;
    if (name != NULL) {
      asprintf(&_name, "%s", ++name);
    } else {
      asprintf(&_name, "%s", path);
    }
  } else {
    asprintf(&_path, "%s/%s", path, name);
    asprintf(&_name, "%s", name);
  }
  metadata(_path);
}

int File2::create() {
  int readfile = open(_path, O_WRONLY | O_CREAT, 0666);

  if (readfile < 0) {
    return -1;
  }
  close(readfile);
  metadata(_path);
  return 0;
}

int Directory::createList() {
  DIR* directory = opendir(_path);
  if (directory == NULL) return -1;

  struct dirent *dir_entry;
  while (((dir_entry = readdir(directory)) != NULL) && ! terminating()) {
    /* Ignore . and .. */
    if (!strcmp(dir_entry->d_name, ".") || !strcmp(dir_entry->d_name, "..")){
      continue;
    }
    Node *g = new Node(_path, dir_entry->d_name);
    list<Node*>::iterator i = _nodes.begin();
    while ((i != _nodes.end()) && (strcmp((*i)->name(), g->name()) < 0)) {
      i++;
    }
    _nodes.insert(i, g);
  }

  closedir(directory);
  return 0;
}

int Directory::parseList() {
  list<Node*>::iterator i = _nodes.begin();
  while (i != _nodes.end()) {
    Node* payload = *i;
    switch (payload->type()) {
      case 'f': {
        File2 *f = new File2(*payload);
        delete *i;
        *i = f;
      }
      break;
      case 'l': {
        Link *l = new Link(*payload);
        delete *i;
        *i = l;
      }
      break;
      case 'd': {
        Directory *d = new Directory(*payload);
        delete *i;
        *i = d;
        if (! d->createList()) {
          d->parseList();
        }
      }
      break;
    }
    i++;
  }
  return 0;
}

void Directory::deleteList() {
  list<Node*>::iterator i = _nodes.begin();
  while (i != _nodes.end()) {
    delete *i;
    i++;
  }
}

int Directory::create() {
  if (isValid()) {
    return 0;
  }
  if (mkdir(_path, 0777)) {
    return -1;
  }
  metadata(_path);
  return 0;
}

File::File(const string& access_path, const string& path) {
  string full_path;
  if (path.empty()) {
    _path     = access_path;
    full_path = _path;
  } else {
    _path     = path;
    full_path = access_path + "/" + _path;
  }
  _link        = "";
  struct stat64 metadata;

  if (lstat64(full_path.c_str(), &metadata)) {
    // errno set by lstat
    _type = '?';
  } else {
    /* Fill in file information */
    if (S_ISREG(metadata.st_mode))       _type =  'f';
    else if (S_ISDIR(metadata.st_mode))  _type =  'd';
    else if (S_ISCHR(metadata.st_mode))  _type =  'c';
    else if (S_ISBLK(metadata.st_mode))  _type =  'b';
    else if (S_ISFIFO(metadata.st_mode)) _type =  'p';
    else if (S_ISLNK(metadata.st_mode))  _type =  'l';
    else if (S_ISSOCK(metadata.st_mode)) _type =  's';
    else                                 _type =  '?';
    if (_type == 'd') {
      _size  = 0;
      _mtime = 0;
    } else {
      _size  = metadata.st_size;
      _mtime = metadata.st_mtime;
    }
    _uid   = metadata.st_uid;
    _gid   = metadata.st_gid;
    _mode  = metadata.st_mode & ~S_IFMT;
    // Get value pointed by link
    if (_type == 'l') {
      char* link = new char[FILENAME_MAX];
      int   size = readlink(full_path.c_str(), link, FILENAME_MAX);

      if (size < 0) {
        _type = '?';
      } else {
        link[size] = '\0';
        _link = link;
      }
      free(link);
    }
  }
  _checksum = "";
}

File::File(char* line, size_t size) {
  char      *start = line;
  char      *delim;
  char      *value = new char[size];
  int       failed = 0;

  // Dummy constructor for file reading
  if (size == 0) return;

  for (int field = 2; field <= 9; field++) {
    // Get tabulation position
    delim = strchr(start, '\t');
    if (delim == NULL) {
      failed = 1;
    } else {
      // Get string portion
      strncpy(value, start, delim - start);
      value[delim - start] = '\0';
      /* Extract data */
      switch (field) {
        case 2:   /* Path */
          _path = value;
          break;
        case 3:   /* Type */
          if (sscanf(value, "%c", &_type) != 1) {
            failed = 2;
          }
          break;
        case 4:   /* Size */
          if (sscanf(value, "%lld", &_size) != 1) {
            failed = 2;
          }
          break;
        case 5:   /* Modification time */
          if (sscanf(value, "%ld", &_mtime) != 1) {
            failed = 2;
          }
          break;
        case 6:   /* User */
          if (sscanf(value, "%u", &_uid) != 1) {
            failed = 2;
          }
          break;
        case 7:   /* Group */
          if (sscanf(value, "%u", &_gid) != 1) {
            failed = 2;
          }
          break;
        case 8:   /* Permissions */
          if (sscanf(value, "%o", &_mode) != 1) {
            failed = 2;
          }
          break;
        case 9:   /* Link */
          _link = value;
      }
      start = delim + 1;
    }
    if (failed) {
      break;
    }
  }
  free(value);
  if (failed != 0) {
    _type = '?';
  }
}

// Tested in db's test
bool File::metadiffer(const File& right) const {
  return (_mtime != right._mtime)   || (_size != right._size)
      || (_uid != right._uid)       || (_gid != right._gid)
      || (_mode != right._mode)     || (_link != right._link);
}

// Tested in db's test
bool File::operator<(const File& right) const {
  if (_path == right._path) {
    return (_mtime < right._mtime)  || (_size < right._size)
        || (_uid < right._uid)      || (_gid < right._gid)
        || (_mode < right._mode)    || (_link < right._link);
  }
  return (_path < right._path);
}

bool File::operator!=(const File& right) const {
  return (_path != right._path) || metadiffer(right);
}

// Tested in cvs_parser's test
string File::name() const {
  unsigned int pos = _path.rfind("/");
  if (pos == string::npos) {
    return _path;
  } else {
    return _path.substr(pos + 1);
  }
}

string File::line(bool nodates) const {
  string  output = _path;
  char*   numbers = NULL;
  time_t  mtime;

  if (nodates) {
    mtime = _mtime != 0;
  } else {
    mtime = _mtime;
  }

  asprintf(&numbers, "%c\t%lld\t%ld\t%u\t%u\t%o",
    _type, _size, mtime, _uid, _gid, _mode);
  output += "\t" + string(numbers) + "\t" + _link;
  delete numbers;
  return output;
}

void Stream::md5sum(char* out, const unsigned char* in, int bytes) {
  char* hex   = "0123456789abcdef";

  while (bytes-- != 0) {
    *out++ = hex[*in >> 4];
    *out++ = hex[*in & 0xf];
    in++;
  }
  *out = '\0';
}

int Stream::open(const char* req_mode, unsigned int compression) {
  _fmode = O_NOATIME | O_LARGEFILE;

  switch (req_mode[0]) {
  case 'w':
    _fmode |= O_WRONLY | O_CREAT | O_TRUNC;
    _size = 0;
    break;
  case 'r':
    _fmode |= O_RDONLY;
    break;
  default:
    errno = EACCES;
    return -1;
  }

  _dsize  = 0;
  _fempty = true;
  _fd = std::open(_path, _fmode, 0666);
  if (_fd < 0) {
    return -1;
  }

  /* Create openssl resources */
  _ctx = new EVP_MD_CTX;
  if (_ctx != NULL) {
    EVP_DigestInit(_ctx, EVP_md5());
  }

  /* Create zlib resources */
  if (compression != 0) {
    _fbuffer        = new unsigned char[chunk];
    _strm           = new z_stream;
    _strm->zalloc   = Z_NULL;
    _strm->zfree    = Z_NULL;
    _strm->opaque   = Z_NULL;
    _strm->avail_in = 0;
    _strm->next_in  = Z_NULL;
    if (_fmode & O_WRONLY) {
      /* Compress */
      if (deflateInit2(_strm, compression, Z_DEFLATED, 16 + 15, 9,
          Z_DEFAULT_STRATEGY)) {
        fprintf(stderr, "stream: deflate init failed\n");
        compression = 0;
      }
    } else {
      /* De-compress */
      if (inflateInit2(_strm, 32 + 15)) {
        compression = 0;
      }
    }
  } else {
    _strm = NULL;
  }

  return _fd == -1;
}

int Stream::close() {
  if (_fd == -1) return -1;

  /* Compute checksum */
  if (_ctx != NULL) {
    unsigned char checksum[36];
    size_t        length;

    EVP_DigestFinal(_ctx, checksum, &length);
    _checksum = (char*) malloc(2 * length + 1);
    md5sum(_checksum, checksum, length);
  }

  /* Destroy zlib resources */
  if (_strm != NULL) {
    if (_fmode & O_WRONLY) {
      deflateEnd(_strm);
    } else {
      inflateEnd(_strm);
    }
  }

  int rc = std::close(_fd);
  _fd = -1;
  metadata(_path);
  return rc;
}

ssize_t Stream::read(unsigned char* buffer, size_t count) {
  ssize_t length;

  if ((_fd == -1) || (_fmode & O_WRONLY)) {
    errno = EINVAL;
    return -1;
  }

  if (count > chunk) count = chunk;
  if (count == 0) return 0;

  if (_strm == NULL) {
    _fbuffer = buffer;
  }

  // Read new data
  if (_fempty) {
    // Read chunk
    length = std::read(_fd, _fbuffer, count);
    if (length < 0) {
      return -1;
    }

    /* Update checksum with chunk */
    if (_ctx != NULL) {
      EVP_DigestUpdate(_ctx, _fbuffer, length);
    }

    /* Fill decompression input buffer with chunk or just return chunk */
    if (_strm != NULL) {
      _strm->avail_in = length;
      _strm->next_in  = _fbuffer;
      _fempty = false;
    } else {
      _dsize += length;
      return length;
    }
  }

  // Continue decompression of previous data
  _strm->avail_out = chunk;
  _strm->next_out  = buffer;
  switch (inflate(_strm, Z_NO_FLUSH)) {
    case Z_NEED_DICT:
    case Z_DATA_ERROR:
    case Z_MEM_ERROR:
      fprintf(stderr, "File::read: inflate failed\n");
  }
  _fempty = (_strm->avail_out != 0);
  length = chunk - _strm->avail_out;
  _dsize += length;
  return length;
}

ssize_t Stream::write(unsigned char* buffer, size_t count, bool eof) {
  static bool finished = true;
  ssize_t length;

  if ((_fd == -1) || ! (_fmode & O_WRONLY) || (count > chunk)) {
    errno = EINVAL;
    return -1;
  }

  // No compression, nothing to finish
  if (_strm == NULL) {
    finished = true;
  }

  if (count == 0) {
    if (finished) {
      return 0;
    }
    finished = true;
  } else {
    finished = false;
  }

  _dsize += count;

  if (_strm == NULL) {
    // Just write
    ssize_t wlength;

    length = count;

    /* Checksum computation */
    if (_ctx != NULL) {
      EVP_DigestUpdate(_ctx, buffer, length);
    }

    do {
      wlength = std::write(_fd, buffer, length);
      length -= wlength;
    } while ((length != 0) && (wlength != 0));
  } else {
    // Compress data
    _strm->avail_in = count;
    _strm->next_in  = buffer;
    count = 0;

    do {
      _strm->avail_out = chunk;
      _strm->next_out  = _fbuffer;
      deflate(_strm, eof ? Z_FINISH : Z_NO_FLUSH);
      length = chunk - _strm->avail_out;
      count += length;

      /* Checksum computation */
      if (_ctx != NULL) {
        EVP_DigestUpdate(_ctx, _fbuffer, length);
      }

      ssize_t wlength;
      do {
        wlength = std::write(_fd, _fbuffer, length);
        length -= wlength;
      } while ((length != 0) && (wlength != 0));
    } while (_strm->avail_out == 0);
  }

  _size += count;
  return count;
}

int Stream::computeChecksum() {
  if (open("r", 0)) {
    return -1;
  }
  unsigned char buffer[Stream::chunk];
  size_t read_size = 0;
  ssize_t size;
  do {
    size = read(buffer, Stream::chunk);
    if (size < 0) {
      break;
    }
    read_size += size;
  } while (size != 0);
  if (close()) {
    return -1;
  }
  if (read_size != _size) {
    errno = EAGAIN;
    return -1;
  }
  return 0;
}

int Stream::copy(Stream& source) {
  if ((_fd == -1) || (source._fd == -1)) {
    errno = EBADF;
  }
  unsigned char buffer[Stream::chunk];
  size_t read_size = 0;
  size_t write_size = 0;
  bool eof = false;
  do {
    ssize_t size = source.read(buffer, Stream::chunk);
    if (size < 0) {
      break;
    }
    eof = (size == 0);
    read_size += size;
    size = write(buffer, size, eof);
    if (size < 0) {
      break;
    }
    write_size += size;
  } while (! eof);
  if (read_size != source.dsize()) {
    errno = EAGAIN;
    return -1;
  }
  return 0;
}

// Public functions
int File::decodeLine(const string& line, vector<string>& params) {
  const char* read  = line.c_str();
  const char* end   = &read[line.size()];
  char* value       = NULL;
  char* write       = NULL;
  bool increment;
  bool skipblanks   = true;
  bool checkcomment = false;
  bool escaped      = false;
  bool unquoted     = false;
  bool singlequoted = false;
  bool doublequoted = false;
  bool valueend     = false;
  bool endedwell    = true;

  while (read <= end) {
    increment = true;
    // End of line
    if (read == end) {
      if (unquoted || singlequoted || doublequoted) {
        // Stop decoding
        valueend = true;
        // Missing closing single- or double-quote
        if (singlequoted || doublequoted) {
          endedwell = false;
        }
      }
    } else
    // Skip blanks until no more, then change mode
    if (skipblanks) {
      if ((*read != ' ') && (*read != '\t')) {
        skipblanks = false;
        checkcomment = true;
        // Do not increment!
        increment = false;
      }
    } else if (checkcomment) {
      if (*read == '#') {
        // Nothing more to do
        break;
      } else {
        checkcomment = false;
        // Do not increment!
        increment = false;
      }
    } else { // neither a blank nor a comment
      if (singlequoted || doublequoted) {
        // Decoding quoted string
        if ((singlequoted && (*read == '\''))
         || (doublequoted && (*read == '"'))) {
          if (escaped) {
            *write++ = *read;
            escaped  = false;
          } else {
            // Found match, stop decoding
            singlequoted = doublequoted = false;
            valueend = true;
          }
        } else if (*read == '\\') {
          escaped = true;
        } else {
          if (escaped) {
            *write++ = '\\';
            escaped  = false;
          }
          *write++ = *read;
        }
      } else if (unquoted) {
        // Decoding unquoted string
        if ((*read == ' ') || (*read == '\t')) {
          // Found blank, stop decoding
          unquoted = false;
          valueend = true;
          // Do not increment!
          increment = false;
        } else {
          *write++ = *read;
        }
      } else {
        // Start decoding new string
        write = value = new char[end - read + 1];
        if (*read == '\'') {
          singlequoted = true;
        } else
        if (*read == '"') {
          doublequoted = true;
        } else {
          unquoted = true;
          // Do not increment!
          increment = false;
        }
      }
    }
    if (valueend) {
      *write++ = '\0';
      params.push_back(string(value));
      delete value;
      skipblanks = true;
      valueend = false;
    }
    if (increment) {
      read++;
    }
  }

  if (! endedwell) {
    return 1;
  }
  return 0;
}
