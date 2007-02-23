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

using namespace std;

#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <cctype>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <openssl/md5.h>
#include <openssl/evp.h>
#include <zlib.h>

#include "files.h"
#include "hbackup.h"

#define CHUNK 409600

File::File(const string& prefix, const string& path) {
  string full_path;
  if (path.empty()) {
    _prefix   = "";
    _path     = prefix;
    full_path = _path;
  } else {
    _prefix   = prefix;
    _path     = path;
    full_path = _prefix + "/" + _path;
  }
  _checksum    = "";
  _link        = "";
  struct  stat metadata;

  if (lstat(full_path.c_str(), &metadata)) {
    // errno set by lstat
    _type = 0;
  } else {
    /* Fill in file information */
    _type  = metadata.st_mode & S_IFMT;
    _mtime = metadata.st_mtime;
    if (S_ISDIR(_type)) {
      _size = 0;
    } else {
      _size = metadata.st_size;
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
}

// Tested in db's test
bool File::metadiffer(const File& right) const {
  return (_type != right._type) || (_mtime != right._mtime)
      || (_size != right._size) || (_uid != right._uid)
      || (_gid != right._gid) || (_mode != right._mode);
}

// Tested in db's test
bool File::operator<(const File& right) const {
  return (_prefix < right._prefix) || (_path < right._path);
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
  string  output = _prefix + "\t" + _path + "\t" + typeLetter(_type);
  char*   numbers = NULL;
  time_t  mtime;

  if (nodates) {
    mtime = _mtime != 0;
  } else {
    mtime = _mtime;
  }

  asprintf(&numbers, "%ld\t%ld\t%u\t%u\t%o", _size, mtime, _uid, _gid, _mode);
  output += "\t" + string(numbers) + "\t" + _link + "\t" + _checksum;
  delete numbers;
  return output;
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
  fstream file(path.c_str(), fstream::in);

  if (! file.is_open()) {
    if (create) {
      file.open(path.c_str(), fstream::out);
      if (! file.is_open()) {
        cerr << "db: failed to create file: " << path << endl;
        return 2;
      }
      file.close();
    }
    return 1;
  }
  file.close();
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
    off_t*        size_in,
    off_t*        size_out,
    string*       checksum_in,
    string*       checksum_out,
    int           compress) {
  FILE          *writefile;
  FILE          *readfile;
  unsigned char buffer_in[CHUNK];
  unsigned char buffer_out[CHUNK];
  EVP_MD_CTX    ctx_in;
  EVP_MD_CTX    ctx_out;
  size_t        length;
  z_stream      strm;

  /* Initialise */
  if (size_in != NULL)  *size_in  = 0;
  if (size_out != NULL) *size_out = 0;

  /* Open file to read from */
  if ((readfile = fopen(source_path.c_str(), "r")) == NULL) {
    return 2;
  }

  /* Open file to write to */
  if ((writefile = fopen(dest_path.c_str(), "w")) == NULL) {
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
    size_t rlength = fread(buffer_in, 1, CHUNK, readfile);
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
        strm.avail_out = CHUNK;
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
        rlength = CHUNK - strm.avail_out;

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
    char buffer[CHUNK];

    rlength = fread(buffer, 1, CHUNK, readfile);
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
