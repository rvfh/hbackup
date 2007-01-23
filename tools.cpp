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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <openssl/md5.h>
#include <openssl/evp.h>
#include <zlib.h>
#include "hbackup.h"
#include "tools.h"

#define CHUNK 10240

void no_trailing_slash(char *string) {
  char *last = &string[strlen(string) - 1];

  while ((last >= string) && (*last == '/')) {
    *last-- = '\0';
  }
}

void strtolower(char *string) {
  char *letter = string;
  while (*letter != '\0') {
    *letter = tolower(*letter);
    letter++;
  }
}

void pathtolinux(char *path) {
  char *letter = path;

  if (path[1] == ':') {
    path[1] = '$';
  }
  while (*letter != '\0') {
    if (*letter == '\\') {
      *letter = '/';
    }
    letter++;
  }
}

int testdir(const char *path, int create) {
  DIR  *directory;

  if ((directory = opendir(path)) == NULL) {
    if (create && mkdir(path, 0777)) {
      fprintf(stderr, "db: failed to create directory: %s\n", path);
      return 2;
    }
    return 1;
  }
  closedir(directory);
  return 0;
}

int testfile(const char *path, int create) {
  FILE  *file;

  if ((file = fopen(path, "r")) == NULL) {
    if (create) {
      if ((file = fopen(path, "w")) == NULL) {
        fprintf(stderr, "db: failed to create file: %s\n", path);
        return 2;
      }
      fclose(file);
    }
    return 1;
  }
  fclose(file);
  return 0;
}

char type_letter(mode_t mode) {
  if (S_ISREG(mode))  return 'f';
  if (S_ISDIR(mode))  return 'd';
  if (S_ISCHR(mode))  return 'c';
  if (S_ISBLK(mode))  return 'b';
  if (S_ISFIFO(mode)) return 'p';
  if (S_ISLNK(mode))  return 'l';
  if (S_ISSOCK(mode)) return 's';
  return '?';
}

mode_t type_mode(char letter) {
  if (letter == 'f') return S_IFREG;
  if (letter == 'd') return S_IFDIR;
  if (letter == 'c') return S_IFCHR;
  if (letter == 'b') return S_IFBLK;
  if (letter == 'p') return S_IFIFO;
  if (letter == 'l') return S_IFLNK;
  if (letter == 's') return S_IFSOCK;
  return 0;
}

static void md5sum(const char *checksum, int bytes) {
  char *hex            = "0123456789abcdef";
  unsigned char *copy  = new unsigned char[bytes];
  unsigned char *read  = copy;
  unsigned char *write = (unsigned char *) checksum;

  memcpy(copy, checksum, bytes);
  while (bytes != 0) {
    *write++ = hex[*read >> 4];
    *write++ = hex[*read & 0xf];
    read++;
    bytes--;
  }
  *write = '\0';
  delete copy;
}

int getdir(const char *db_path, const char *checksum, char **path_p) {
  const char  *checksumpart = checksum;
  char        *dir_path = NULL;
  int         status;
  int         failed = 0;

  asprintf(&dir_path, "%s/data", db_path);
  /* Two cases: either there are files, or a .nofiles file and directories */
  do {
    char *temp_path = NULL;

    /* If we can find a .nofiles file, then go down one more directory */
    asprintf(&temp_path, "%s/.nofiles", dir_path);
    status = testfile(temp_path, 0);
    free(temp_path);

    if (! status) {
      char *new_dir_path = NULL;

      asprintf(&new_dir_path, "%s/%c%c", dir_path, checksumpart[0],
        checksumpart[1]);
      checksumpart += 2;
      free(dir_path);
      dir_path = new_dir_path;
    }
  } while (! status);
  /* Return path */
  asprintf(path_p, "%s/%s", dir_path, (char *) checksumpart);
  if (testdir(dir_path, 1)) {
    failed = 1;
  }
  free(dir_path);
  return failed;
}

int zcopy(const char *source_path, const char *dest_path,
    off_t *size_in, off_t *size_out, char *checksum_in, char *checksum_out,
    int compress) {
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
  if ((readfile = fopen(source_path, "r")) == NULL) {
    return 2;
  }

  /* Open file to write to */
  if ((writefile = fopen(dest_path, "w")) == NULL) {
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
    EVP_DigestFinal(&ctx_in, (unsigned char *) checksum_in, &length);
    md5sum(checksum_in, length);
  }

  /* Destroy zlib resources */
  if (compress != 0) {
    /* Get checksum for output file */
    if (checksum_out != NULL) {
      EVP_DigestFinal(&ctx_out, (unsigned char *) checksum_out, &length);
      md5sum(checksum_out, length);
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
        strcpy(checksum_out, checksum_in);
      } else {
        EVP_DigestFinal(&ctx_in, (unsigned char *) checksum_out, &length);
        md5sum(checksum_out, length);
      }
    }
  }
  return 0;
}

int getchecksum(const char *path, const char *checksum) {
  FILE       *readfile;
  EVP_MD_CTX ctx;
  size_t     rlength;

  /* Open file */
  if ((readfile = fopen(path, "r")) == NULL) {
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
  EVP_DigestFinal(&ctx, (unsigned char *) checksum, &rlength);
  md5sum(checksum, rlength);

  return 0;
}
