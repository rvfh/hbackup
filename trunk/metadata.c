/* Herve Fache

20061008 Creation
*/

#include <string.h>
#include "metadata.h"

int metadata_get(const char *path, metadata_t *metadata_p) {
  struct stat metadata;

  if (lstat(path, &metadata)) {
    fprintf(stderr, "metada: cannot get file info: %s\n", path);
    return 1;
  }
  /* Fill in file information */
  metadata_p->type  = metadata.st_mode & S_IFMT;
  metadata_p->mtime = metadata.st_mtime;
  metadata_p->size  = metadata.st_size;
  metadata_p->uid   = metadata.st_uid;
  metadata_p->gid   = metadata.st_gid;
  metadata_p->mode  = metadata.st_mode & ~S_IFMT;
  return 0;
}
