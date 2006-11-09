/* Herve Fache

20061005 Creation
*/

#ifndef PARSERS_H
#define PARSERS_H

#include "filelist.h"

/* Our enums */
typedef enum {
  parser_disabled,
  parser_controlled,
  parser_modified,
  parser_modifiedandothers,
  parser_others
} parser_mode_t;

typedef enum {
  parser_dir_unknown,
  parser_dir_controlled,
  parser_dir_other
} parser_dir_status_t;

typedef enum {
  parser_file_unknown,
  parser_file_maybemodified,
  parser_file_controlled,
  parser_file_modified,
  parser_file_other
} parser_file_status_t;

/* Type for function to check if a directory is under control and enter it */
typedef parser_dir_status_t (parsers_dir_check_f) (void **handle,
  const char *dir_path);

/* Type for function to leave a directory */
typedef void (parsers_dir_leave_f) (void *handle);

/* Type for function to check whether a file is under control */
typedef parser_file_status_t (parsers_file_check_f) (void *handle,
  const filedata_t *file_data);

/* Our data */
typedef struct {
  parsers_dir_check_f   *dir_check;
  parsers_dir_leave_f   *dir_leave;
  parsers_file_check_f  *file_check;
  parser_mode_t         mode;
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
