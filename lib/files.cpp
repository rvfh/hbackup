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

using namespace std;

#include "files.h"
#include "hbackup.h"

using namespace hbackup;

GenericFile::GenericFile(const char* path, const char* name) {
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

void GenericFileListElement::insert(GenericFileListElement** first) {
  GenericFileListElement* next     = *first;
  GenericFileListElement* previous = NULL;
  while ((next != NULL)
      && (strcmp(next->_payload->name(), this->_payload->name()) < 0)) {
    previous = next;
    next = next->_next;
  }
  _previous = previous;
  if (previous != NULL) {
    previous->_next = this;
  } else {
    *first = this;
  }
  _next = next;
  if (next != NULL) {
    next->_previous = this;
  }
}

void GenericFileListElement::remove(GenericFileListElement** first) {
  if (_previous != NULL) {
    _previous->_next = _next;
  } else {
    *first = _next;
  }
  if (_next != NULL) {
    _next->_previous = _previous;
  }
}

int Directory::createList(const char* path) {
  DIR* directory = opendir(path);
  if (directory == NULL) return -1;

  struct dirent *dir_entry;
  while (((dir_entry = readdir(directory)) != NULL) && ! terminating()) {
    /* Ignore . and .. */
    if (!strcmp(dir_entry->d_name, ".") || !strcmp(dir_entry->d_name, "..")){
      continue;
    }
    GenericFile *g = new GenericFile(path, dir_entry->d_name);
    GenericFileListElement* e = new GenericFileListElement(g);
    e->insert(&_first_entry);
    _entries++;
  }

  closedir(directory);
  return 0;
}

void Directory::deleteList() {
  GenericFileListElement* current = _first_entry;

  while (current != NULL) {
    GenericFileListElement* next = current->next();
    delete current;
    current = next;
    _entries--;
  }
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
    _type = 0;
  } else {
    /* Fill in file information */
    _type  = metadata.st_mode & S_IFMT;
    if (S_ISDIR(_type)) {
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
    if (S_ISLNK(_type)) {
      char* link = new char[FILENAME_MAX];
      int   size = readlink(full_path.c_str(), link, FILENAME_MAX);

      if (size < 0) {
        _type = 0;
      } else {
        link[size] = '\0';
        _link = link;
      }
      free(link);
    }
  }
  _fd = NULL;
  _checksum = "";
}

File::File(char* line, size_t size) {
  char      *start = line;
  char      *delim;
  char      *value = new char[size];
  char      letter;
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
          if (sscanf(value, "%c", &letter) != 1) {
            failed = 2;
          }
          _type = File::typeMode(letter);
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
    _type = 0;
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
  string  output = _path + "\t" + typeLetter(_type);
  char*   numbers = NULL;
  time_t  mtime;

  if (nodates) {
    mtime = _mtime != 0;
  } else {
    mtime = _mtime;
  }

  asprintf(&numbers, "%lld\t%ld\t%u\t%u\t%o", _size, mtime, _uid, _gid, _mode);
  output += "\t" + string(numbers) + "\t" + _link;
  delete numbers;
  return output;
}

int File::open(const char* prepath, const char* req_mode,
    unsigned int compression) {
  char mode[2];

  switch (req_mode[0]) {
  case 'w':
    strcpy(mode, "w");
    _fwrite = true;
    _size = 0;
    break;
  case 'r':
    strcpy(mode, "r");
    _fwrite = false;
    break;
  default:
    return 1;
  }

  string path;
  if (prepath[0] == '\0') {
    path = _path;
  } else {
    path = string(prepath) + "/" + _path;
  }

  _dsize  = 0;
  _fempty = true;
  _fd = fopen64(path.c_str(), mode);
  if (_fd == NULL)
    return -1;
  if (feof(_fd)) {
    _feof = true;
  } else {
    _feof = false;
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
    if (_fwrite) {
      /* Compress */
      if (deflateInit2(_strm, compression, Z_DEFLATED, 16 + 15, 9,
          Z_DEFAULT_STRATEGY)) {
        fprintf(stderr, "zcopy: deflate init failed\n");
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

  return _fd == NULL;
}

int File::close() {
  if (_fd == NULL) return -1;

  /* Compute checksum */
  if (_ctx != NULL) {
    unsigned char checksum[36];
    size_t        length;

    EVP_DigestFinal(_ctx, checksum, &length);
    md5sum(_checksum, checksum, length);
  }

  /* Destroy zlib resources */
  if (_strm != NULL) {
    if (_fwrite) {
      deflateEnd(_strm);
    } else {
      inflateEnd(_strm);
    }
  }

  return fclose(_fd);
}

ssize_t File::read(unsigned char* buffer, size_t count) {
  size_t length;

  if ((_fd == NULL) || _fwrite) {
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
    length = fread(_fbuffer, 1, count, _fd);

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
      if (length == 0)
        _feof = true;
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
      break;
    case Z_STREAM_END:
      _feof = true;
      break;
    default:
      _feof = false;
  }
  _fempty = (_strm->avail_out != 0);
  length = chunk - _strm->avail_out;
  _dsize += length;
  return length;
}

ssize_t File::write(unsigned char* buffer, size_t count, bool eof) {
  static bool finished = true;
  size_t length;

  if ((_fd == NULL) || ! _fwrite || (count > chunk)) {
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
    size_t wlength;

    length = count;

    /* Checksum computation */
    if (_ctx != NULL) {
      EVP_DigestUpdate(_ctx, buffer, length);
    }

    do {
      wlength = fwrite(buffer, 1, length, _fd);
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

      size_t wlength;
      do {
        wlength = fwrite(_fbuffer, 1, length, _fd);
        length -= wlength;
      } while ((length != 0) && (wlength != 0));
    } while (_strm->avail_out == 0);
  }

  _size += count;
  return count;
}

ssize_t File::readLine(unsigned char* buffer, size_t count, char delim) {
  return 0;
}

ssize_t File::writeLine(const unsigned char* buffer, size_t count, char delim){
  return 0;
}

ssize_t File::readParams(vector<string>& params) {
  return 0;
}

// Public functions
int File::testDir(const string& path, bool create) {
  DIR  *directory;

  if ((directory = opendir(path.c_str())) == NULL) {
    if (create && mkdir(path.c_str(), 0777)) {
      cerr << "Failed to create directory: " << path << endl;
      return 2;
    }
    return 1;
  }
  closedir(directory);
  return 0;
}

int File::testReg(const string& path, bool create) {
  // Don't use C++ stuff: no errno set
  FILE  *readfile;

  if ((readfile = fopen(path.c_str(), "r")) == NULL) {
    // File does not exist
    if (create) {
      if ((readfile = fopen(path.c_str(), "w")) == NULL) {
        cerr << "Failed to create file: " << path << endl;
        return 2;
      }
      fclose(readfile);
    }
    return 1;
  }
  fclose(readfile);
  return 0;
}

char File::typeLetter(mode_t mode) {
  if (S_ISREG(mode))  return 'f';
  if (S_ISDIR(mode))  return 'd';
  if (S_ISCHR(mode))  return 'c';
  if (S_ISBLK(mode))  return 'b';
  if (S_ISFIFO(mode)) return 'p';
  if (S_ISLNK(mode))  return 'l';
  if (S_ISSOCK(mode)) return 's';
  if (mode == 0)      return '!';
  return '?';
}

mode_t File::typeMode(char letter) {
  if (letter == 'f') return S_IFREG;
  if (letter == 'd') return S_IFDIR;
  if (letter == 'c') return S_IFCHR;
  if (letter == 'b') return S_IFBLK;
  if (letter == 'p') return S_IFIFO;
  if (letter == 'l') return S_IFLNK;
  if (letter == 's') return S_IFSOCK;
  return 0;
}

void File::md5sum(
    string&               checksum_out,
    const unsigned char*  checksum_in,
    int                   bytes) {
  char* hex   = "0123456789abcdef";
  char* copy  = new char[2 * bytes + 1];
  char* write = copy;

  while (bytes != 0) {
   *write++ = hex[*checksum_in >> 4];
   *write++ = hex[*checksum_in & 0xf];
    checksum_in++;
    bytes--;
  }
  *write = '\0';
  checksum_out = copy;
  delete copy;
}

int File::zcopy(
    const string& source_path,
    const string& dest_path,
    long long*    size_in,
    long long*    size_out,
    string*       checksum_in,
    string*       checksum_out,
    int           compress) {
  FILE          *writefile;
  FILE          *readfile;
  unsigned char buffer_in[chunk];
  unsigned char buffer_out[chunk];
  EVP_MD_CTX    ctx_in;
  EVP_MD_CTX    ctx_out;
  size_t        length;
  z_stream      strm;

  /* Initialise */
  if (size_in != NULL)  *size_in  = 0;
  if (size_out != NULL) *size_out = 0;

  /* Open file to read from */
  if ((readfile = fopen64(source_path.c_str(), "r")) == NULL) {
    return 2;
  }

  /* Open file to write to */
  if ((writefile = fopen64(dest_path.c_str(), "w")) == NULL) {
    fclose(readfile);
    return 2;
  }

  /* Create zlib resources */
  if (compress != 0) {
    /* Create openssl resources */
    EVP_DigestInit(&ctx_out, EVP_md5());

    strm.zalloc   = Z_NULL;
    strm.zfree    = Z_NULL;
    strm.opaque   = Z_NULL;
    strm.avail_in = 0;
    strm.next_in  = Z_NULL;
    if (compress > 0) {
      /* Compress */
      if (deflateInit2(&strm, compress, Z_DEFLATED, 16 + 15, 9,
          Z_DEFAULT_STRATEGY)) {
        fprintf(stderr, "zcopy: deflate init failed\n");
        compress = 0;
      }
    } else {
      /* De-compress */
      if (inflateInit2(&strm, 32 + 15)) {
        compress = 0;
      }
    }
  }

  /* Create openssl resources */
  EVP_DigestInit(&ctx_in, EVP_md5());

  /* We shall copy, (de-)compress and compute the checksum in one go */
  while (! feof(readfile) && ! terminating()) {
    size_t rlength = fread(buffer_in, 1, chunk, readfile);
    size_t wlength;

    /* Size read */
    if (size_in != NULL) *size_in += rlength;

    /* Checksum computation */
    EVP_DigestUpdate(&ctx_in, buffer_in, rlength);

    /* Compression */
    if (compress != 0) {
      strm.avail_in = rlength;
      strm.next_in = buffer_in;

      do {
        strm.avail_out = chunk;
        strm.next_out = buffer_out;
        if (compress > 0) {
          deflate(&strm, feof(readfile) ? Z_FINISH : Z_NO_FLUSH);
        } else {
          switch (inflate(&strm, Z_NO_FLUSH)) {
            case Z_NEED_DICT:
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
              fprintf(stderr, "zcopy: inflate failed\n");
              break;
          }
        }
        rlength = chunk - strm.avail_out;

        /* Checksum computation */
        EVP_DigestUpdate(&ctx_out, buffer_out, rlength);

        /* Size to write */
        if (size_out != NULL) *size_out += rlength;

        do {
          wlength = fwrite(buffer_out, 1, rlength, writefile);
          rlength -= wlength;
        } while ((rlength != 0) && (wlength != 0));
      } while (strm.avail_out == 0);
    } else {
      /* Size to write */
      if (size_out != NULL) *size_out += rlength;

      do {
        wlength = fwrite(buffer_in, 1, rlength, writefile);
        rlength -= wlength;
      } while ((rlength != 0) && (wlength != 0));
    }
  }
  fclose(readfile);
  fclose(writefile);

  /* Get checksum for input file */
  if (checksum_in != NULL) {
    unsigned char checksum[36];
    EVP_DigestFinal(&ctx_in, checksum, &length);
    md5sum(*checksum_in, checksum, length);
  }

  /* Destroy zlib resources */
  if (compress != 0) {
    /* Get checksum for output file */
    if (checksum_out != NULL) {
      unsigned char checksum[36];
      EVP_DigestFinal(&ctx_out, checksum, &length);
      md5sum(*checksum_out, checksum, length);
    }

    if (compress > 0) {
      deflateEnd(&strm);
    } else
    if (compress < 0) {
      inflateEnd(&strm);
    }
  } else {
    /* Might want the original checksum in the output */
    if (checksum_out != NULL) {
      if (checksum_in != NULL) {
        *checksum_out = *checksum_in;
      } else {
        unsigned char checksum[36];
        EVP_DigestFinal(&ctx_in, checksum, &length);
        md5sum(*checksum_out, checksum, length);
      }
    }
  }
  return 0;
}

int File::getChecksum(const string& path, string& checksum) {
  FILE       *readfile;
  EVP_MD_CTX ctx;
  size_t     rlength;

  /* Open file */
  if ((readfile = fopen(path.c_str(), "r")) == NULL) {
    return 2;
  }

  /* Initialize checksum calculation */
  EVP_DigestInit(&ctx, EVP_md5());

  /* Read file, updating checksum */
  while (! feof(readfile) && ! terminating()) {
    char buffer[chunk];

    rlength = fread(buffer, 1, chunk, readfile);
    EVP_DigestUpdate(&ctx, buffer, rlength);
  }
  fclose(readfile);

  /* Get checksum */
  unsigned char checksum_temp[36];
  EVP_DigestFinal(&ctx, checksum_temp, &rlength);
  md5sum(checksum, checksum_temp, rlength);

  return 0;
}

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
