/* Herve Fache

20061008 Creation
*/

#ifndef METADATA_H
#define METADATA_H

#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
/* Declare lstat and S_* macros */
#ifndef __USE_BSD
#define __USE_BSD
#endif
#include <sys/stat.h>
#include <unistd.h>
#include "list.h"

/* Our data */
typedef struct {
  char   path[FILENAME_MAX];
  mode_t type;    /* type */
  time_t mtime;   /* time of last modification */
  off_t  size;    /* total size, in bytes */
  uid_t  uid;     /* user ID of owner */
  gid_t  gid;     /* group ID of owner */
  mode_t mode;    /* permissions */
} metadata_t;

extern int metadata_get(const char *path, metadata_t *file_data_p);

#endif
