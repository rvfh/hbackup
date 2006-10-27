/* Herve Fache

20061025 Creation
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>
#include "tools.h"

void one_trailing_slash(char *string) {
  char *last = &string[strlen(string) - 1];

  while ((last >= string) && (*last == '/')) {
    *last-- = '\0';
  }
  *++last = '/';
  *++last = '\0';
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
