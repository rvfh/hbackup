/* Herve Fache

20061003 Creation
*/

#ifndef FILE_LIST_H
#define FILE_LIST_H

#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
/* Declare lstat and S_* macros */
#ifndef __USE_BSD
#define __USE_BSD
#endif
#include <sys/stat.h>
#include <unistd.h>
#include "metadata.h"

typedef struct {
  char        *path;
  char        checksum[36];
  metadata_t  metadata;
} filedata_t;

/* Create list of files from path, using filters */
extern int filelist_new(const char *path, list_t filters, list_t parsers);

/* Destroy list of files */
extern void filelist_free(void);

/* Obtain list of files */
extern list_t filelist_get(void);

#endif
