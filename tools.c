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

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>
#include "tools.h"

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
