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

#ifndef TOOLS_H
#define TOOLS_H

#include <sys/types.h>

// Ring buffer with simultaneous read and write operations support
template<class T>
class RingBuffer {
  T   *_start;
  T   *_end;
  T   *_read;
  T   *_write;
  int _size;
  int _free;
public:
  // Create a buffer of given size
  RingBuffer(int size = 512);
  // Destroy buffer
  ~RingBuffer();
  // Write size elements contained in data.
  // Returns actual size written
  int write(const T *data, int size);
  // Read size elements into data.
  // Returns actual size read
  int read(T *data, int size);
  // Returns buffer used size
  int size();
};

/* Make sure string finishes without slash */
extern void no_trailing_slash(string&);

/* Convert string to lower case */
extern void strtolower(string&);

/* Convert path to UNIX style */
extern void pathtolinux(string&);

/* Test whether dir exists, create it if requested */
extern int testdir(const string& path, bool create);

/* Test whether file exists, create it if requested */
extern int testfile(const string& path, bool create);

extern char type_letter(mode_t mode);

extern mode_t type_mode(char letter);

extern int getdir(const string& db_path, const string &checksum, string& path);

extern int zcopy(
  const string& source_path,
  const string& dest_path,
  off_t         *size_in,
  off_t         *size_out,
  char          *checksum_in,
  char          *checksum_out,
  int           compress);

extern int getchecksum(const string& path, const char *checksum);

/* Read parameters from line */
extern int params_readline(string line, char *keyword, char *type,
  string *);

#endif
