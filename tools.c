/* Herve Fache

20061025 Creation
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include "tools.h"

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
