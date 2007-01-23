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

/* Create a list of files to be backed up

Algorithm for temporary list creation:
 is directory under known SCM control
  yes: is file reported added or modified
   yes: add to temporary list of files to be backed up
   no: nothing
  no: for each element of current directory
   is it to be ignored?
    yes: nothing
    no: is it a directory?
     yes: add to temporary list of files to be backed up and descend into it
     no: add to temporary list of files to be backed up
*/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include "hbackup.h"
#include "list.h"
#include "metadata.h"
#include "filters.h"
#include "parsers.h"
#include "tools.h"
#include "filelist.h"

/* Mount point string length */
static int mount_path_length = 0;

static list_t       *files          = NULL;
static const list_t *filters_handle = NULL;
static const list_t *parsers_handle = NULL;

static char *filedata_get(const void *payload) {
  const filedata_t *filedata = (const filedata_t *) (payload);
  char *string = NULL;

  asprintf(&string, "%s", filedata->path);
  return string;
}

static int iterate_directory(const char *path, parser_t *parser) {
  void          *handle;
  filedata_t    filedata;
  filedata_t    *filedata_p;
  DIR           *directory;
  struct dirent *dir_entry;

  if (verbosity() > 2) {
    printf(" --> Dir: %s\n", &path[mount_path_length]);
  }
  /* Check whether directory is under SCM control */
  if (parsers_dir_check(parsers_handle, &parser, &handle, path) == 2) {
    return 0;
  }
  directory = opendir(path);
  if (directory == NULL) {
    fprintf(stderr, "filelist: cannot open directory: %s\n", path);
    return 2;
  }
  while (((dir_entry = readdir(directory)) != NULL) && ! terminating()) {
    char *file_path;
    int  failed = 0;

    /* Ignore . and .. */
    if (! strcmp(dir_entry->d_name, ".") || ! strcmp(dir_entry->d_name, "..")){
      continue;
    }
    asprintf(&file_path, "%s/%s", path, dir_entry->d_name);
    /* Remove mount path and leading slash from records */
    asprintf(&filedata.path, "%s", &file_path[mount_path_length + 1]);
    strcpy(filedata.checksum, "");
    if (metadata_get(file_path, &filedata.metadata)) {
      fprintf(stderr, "filelist: cannot get metadata: %s\n", file_path);
      failed = 2;
    } else
    /* Let the parser analyse the file data to know whether to back it up */
    if (parsers_file_check(parser, handle, &filedata)) {
      failed = 1;
    } else
    /* Now pass it through the filters */
    if ((filters_handle != NULL) && ! filters_match(filters_handle,
        &filedata)) {
      failed = 1;
    } else
    if (S_ISDIR(filedata.metadata.type)) {
      filedata.metadata.size = 0;
      if (iterate_directory(file_path, parser)) {
        if (! terminating()) {
          fprintf(stderr, "filelist: cannot iterate into directory: %s\n",
            dir_entry->d_name);
        }
        failed = 2;
      }
    }
    free(file_path);
    if (failed) {
      free(filedata.path);
    } else {
      filedata_p = (filedata_t *) (malloc(sizeof(filedata_t)));
      *filedata_p = filedata;
      list_add(files, filedata_p);
    }
  }
  closedir(directory);
  if (terminating()) {
    return 1;
  }
  return 0;
}

int filelist_new(const char *path, const list_t *filters, const list_t *parsers) {
  filters_handle = filters;
  parsers_handle = parsers;
  files = list_new(filedata_get);
  if (files == NULL) {
    fprintf(stderr, "filelist: new: cannot initialise\n");
    return 2;
  }
  mount_path_length = strlen(path);
  if (iterate_directory(path, NULL)) {
    filelist_free();
    return 1;
  }
  return 0;
}

void filelist_free(void) {
  list_entry_t *entry = NULL;

  while ((entry = list_next(files, entry)) != NULL) {
    filedata_t *filedata = (filedata_t *) (list_entry_payload(entry));

    free(filedata->path);
  }
  list_free(files);
}

list_t *filelist_get(void) {
  return files;
}

