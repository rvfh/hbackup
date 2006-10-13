/* Herve Fache

20061008 Creation
*/

#ifndef FILTERS_H
#define FILTERS_H

/* Our data */
typedef enum {
  filter_end,         /* End of path/file name */
  filter_path_start,  /* Start of path */
  filter_path_regexp, /* Regular expression on path */
  filter_file_start,  /* Start of file name */
  filter_file_regexp  /* Regular expression on file name */
} filter_type_t;

/* Create filters list */
extern int filters_new(void **handle);

/* Add to list of filters */
/* TODO: specify which type of files (dir, pipe, reg, link) this applies to */
extern int filters_add(void *handle, const char *string, filter_type_t type);

/* Destroy filters list and all filters */
extern void filters_free(void *handle);

/* Get next filter (first if handle is NULL) */
/* TODO: maybe one day we could look at all metadata? Filter on size, date */
extern int filters_match(void *handle, const char *path);

#endif
