/* Herve Fache

20061008 Creation
*/

#ifndef FILTERS_H
#define FILTERS_H

#include "filelist.h"

/* Filter types */
typedef enum {
  filter_path_end,    /* End of file name */
  filter_path_start,  /* Start of file name */
  filter_path_regexp, /* Regular expression on file name */
  filter_size_above,  /* Minimum size (only applies to regular files) */
  filter_size_below   /* Maximum size (only applies to regular files) */
} filter_type_t;

/* Create filters list */
extern int filters_new(void **handle);

/* Add to list of filters */
extern int filters_add(void *handle, mode_t file_type, filter_type_t 
  filter_type, ...);

/* Destroy filters list and all filters */
extern void filters_free(void *handle);

/* Get next filter (first if handle is NULL) */
extern int filters_match(void *handle, const filedata_t *filedata);

#endif
