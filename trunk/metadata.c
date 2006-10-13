/* Herve Fache

20061008 Creation
*/

#include <string.h>
#include "metadata.h"

int metadata_get(const char *path, metadata_t *file_data_p) {
  struct stat metadata;

  if (lstat(path, &metadata)) {
    fprintf(stderr, "metada: cannot get file info: %s\n", path);
    return 1;
  }
  /* Fill in file information */
  strcpy(file_data_p->path, path);
  file_data_p->type  = metadata.st_mode & S_IFMT;
  file_data_p->mtime = metadata.st_mtime;
  file_data_p->size  = metadata.st_size;
  file_data_p->uid   = metadata.st_uid;
  file_data_p->gid   = metadata.st_gid;
  file_data_p->mode  = metadata.st_mode & ~S_IFMT;
  return 0;
}
