/* Herve Fache

20061005 Creation
*/

#ifndef PARSERS_H
#define PARSERS_H

#include "filelist.h"

/* Type for function to check if a directory is under control and enter it */
typedef int (parsers_dir_check_f) (void **handle, const char *dir_path);

/* Type for function to leave a directory */
typedef void (parsers_dir_leave_f) (void *handle);

/* Type for function to check whether a file is under control */
typedef int (parsers_file_check_f) (void *handle, const filedata_t *file_data);

/* Our data */
typedef struct {
  parsers_dir_check_f   *dir_check;
  parsers_dir_leave_f   *dir_leave;
  parsers_file_check_f  *file_check;
  char                  name[32];
} parser_t;

/* Create parsers list */
extern int parsers_new(void **handle);

/* Add to list of parsers */
extern int parsers_add(void *handle, parser_t *parser);

/* Destroy parsers list and all parsers */
extern void parsers_free(void *handle);

/* Get next parser (first if handle is NULL) */
extern parser_t *parsers_next(void *handle, void **parser_handle);

#endif
