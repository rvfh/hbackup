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
  char        path[FILENAME_MAX];
  char        checksum[36];
  metadata_t  metadata;
} filedata_t;

/* Create list of files from path, using filters */
extern int filelist_new(const char *path, list_t filters, list_t parsers);

/* Destroy list of files */
extern void filelist_free(void);

/* Obtain list of files */
extern list_t filelist_getlist(void);

/* Obtain files startup path (mount point) */
extern const char *filelist_getpath(void);

#endif
