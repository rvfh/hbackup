/*
     Copyright (C) 2006  Herve Fache

     This program is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License version 2 as
     published by the Free Software Foundation.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place - Suite 330,
     Boston, MA 02111-1307, USA.
*/

#ifndef PARSERS_H
#define PARSERS_H

#include "list.h"
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
typedef parser_dir_status_t (parsers_dir_check_f) (void **dir_handle,
  const char *dir_path);

/* Type for function to leave a directory */
typedef void (parsers_dir_leave_f) (void *dir_handle);

/* Type for function to check whether a file is under control */
typedef parser_file_status_t (parsers_file_check_f) (void *dir_handle,
  const filedata_t *filedata);

/* Our data */
typedef struct {
  parsers_dir_check_f   *dir_check;
  parsers_dir_leave_f   *dir_leave;
  parsers_file_check_f  *file_check;
  parser_mode_t         mode;
  char                  name[32];
} parser_t;

/* Create parsers list */
extern int parsers_new(List **handle);

/* Add to list of parsers */
extern int parsers_add(List *handle, parser_mode_t mode, parser_t *parser);

/* Destroy parsers list and all parsers */
extern void parsers_free(List *handle);

/* Check whether [a] parser matches. Returns:
 * 0: [a] parser matches (if *parser_handle not null, only test that parser)
 * 1: no parser matches
 * 2: *parser_handle not null but does not match
 */
extern int parsers_dir_check(const void *parsers_handle,
  parser_t **parser_handle, void **dir_handle, const char *path);

/* Check whether [a] parser matches. Returns:
 * 0: match (use)
 * 1: no match (ignore)
 */
extern int parsers_file_check(parser_t *parser_handle, void *dir_handle,
  const filedata_t *filedata);

#endif
