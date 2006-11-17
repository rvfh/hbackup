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

/* Make sure string finishes without slash */
extern void no_trailing_slash(char *string);

/* Convert string to lower case */
extern void strtolower(char *string);

/* Convert path to UNIX style */
extern void pathtolinux(char *path);

/* Test whether dir exists, create it if requested */
extern int testdir(const char *path, int create);

/* Test whether file exists, create it if requested */
extern int testfile(const char *path, int create);

extern char type_letter(mode_t mode);

extern mode_t type_mode(char letter);

extern int getdir(const char *db_path, const char *checksum, char **path_p);

extern int zcopy(
  const char *source_path,
  const char *dest_path,
  size_t *size_in,
  size_t *size_out,
  char *checksum_in,
  char *checksum_out,
  int compress);

extern int getchecksum(const char *path, const char *checksum);

#endif
