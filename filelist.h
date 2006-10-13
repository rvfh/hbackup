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

/* Create list of files from path, using filters */
extern int file_list_new(const char *path, void *filters);

/* Destroy list of files */
extern void file_list_free(void);

/* Obtain list of files */
extern void *file_list_get(void);

#endif
