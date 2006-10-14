/* Herve Fache

20061005 Creation
*/

#ifndef PARSERS_H
#define PARSERS_H

#include "metadata.h"

/* Type for function to check if a directory is under control and enter it */
typedef int (parsers_dir_check_f) (void **handle, const char *dir_path);

/* Type for function to leave a directory */
typedef void (parsers_dir_leave_f) (void *handle);

/* Type for function to check whether a file is under control */
typedef int (parsers_file_check_f) (void *handle, const metadata_t *file_data);

/* Function to destroy the parser */
typedef void (parsers_destroy_f) (void);

/* Our data */
typedef struct {
  parsers_dir_check_f   *dir_check;
  parsers_dir_leave_f   *dir_leave;
  parsers_file_check_f  *file_check;
  parsers_destroy_f     *destroy;
} parser_t;

/* Create parsers list */
extern int parsers_new(void);

/* Add to list of parsers */
extern int parsers_add(parser_t *parser);

/* Destroy parsers list and all parsers */
extern void parsers_free(void);

/* Get next parser (first if handle is NULL) */
extern parser_t *parsers_next(void **handle);

#endif