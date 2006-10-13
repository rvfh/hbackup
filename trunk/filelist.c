/* Herve Fache

20061003 Creation
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include "list.h"
#include "metadata.h"
#include "filters.h"
#include "parsers.h"
#include "filelist.h"

static list_t file_list = NULL;
static void *filters_handle = NULL;

static void file_data_get(const void *payload_p, char *string) {
  const metadata_t *metadata = payload_p;

  sprintf(string, "%s", metadata->path);
}

static metadata_t * file_list_entry_new(const metadata_t *file_data) {
  metadata_t *file_data_p = malloc(sizeof(metadata_t));
  *file_data_p = *file_data;
  return file_data_p;
}

static int iterate_directory(const char *path, parser_t *parser) {
  void *handle;
  metadata_t file_data;
  DIR *directory;
  struct dirent *dir_entry;

  /* Check whether directory is under SCM control */
  if (parser == NULL) {
/*    list_entry_t *entry = NULL;

    while ((entry = list_next(parsers_list, entry)) != NULL) {
      if (! ((parser_t *) entry->payload)->dir_check(&handle, path)) {
        parser = (parser_t *) entry->payload;
        break;
      }
    }*/
    void *parser_handle = NULL;

    while ((parser = parsers_next(&parser_handle)) != NULL) {
      if (! parser->dir_check(&handle, path)) {
        break;
      }
    }
  } else {
    if (parser->dir_check(&handle, path)) {
      return 0;
    }
  }
  directory = opendir(path);
  if (directory == NULL) {
    fprintf(stderr, "filelist: cannot open directory: %s\n", path);
    return 2;
  }
  while ((dir_entry = readdir(directory)) != NULL) {
    char d_path[FILENAME_MAX];

    /* Ignore . and .. */
    if (! strcmp(dir_entry->d_name, ".") || ! strcmp(dir_entry->d_name, "..")) {
      continue;
    }
    strcpy(d_path, path);
    strcat(d_path, "/");
    strcat(d_path, dir_entry->d_name);
    if (metadata_get(d_path, &file_data)) {
      fprintf(stderr, "filelist: cannot get metadata: %s\n", d_path);
      return 2;
    }
    /* Let the parser analyse the file data to know whether to back it up */
    if ((parser != NULL) && parser->file_check(handle, &file_data)) {
      continue;
    }
    /* Now pass it through the filters */
    if ((filters_handle != NULL) && ! filters_match(filters_handle, file_data.path)) {
      continue;
    }
    if (S_ISDIR(file_data.type)) {
      if (iterate_directory(d_path, parser)) {
        fprintf(stderr, "filelist: cannot iterate into directory: %s\n", dir_entry->d_name);
        return 2;
      }
    }
    list_add(file_list, file_list_entry_new(&file_data));
  }
  closedir(directory);
  return 0;
}

static int file_list_build(const char *path) {
/*  metadata_t file_data;

  if (metadata_get(path, &file_data)) {
    fprintf(stderr, "filelist: build: cannot get metadata: %s\n", path);
    return 2;
  }
  list_add(file_list, file_list_entry_new(&file_data));*/
  return iterate_directory(path, NULL);
}

int file_list_new(const char *path, void *filters) {
  char dir_path[FILENAME_MAX];
  char *path_end = dir_path;

  filters_handle = filters;
  file_list = list_new(file_data_get);
  if (file_list == NULL) {
    fprintf(stderr, "filelist: new: cannot initialise\n");
    return 2;
  }
  /* Remove trailing slashes */
  strcpy(dir_path, path);
  path_end += strlen(dir_path) - 1;
  while ((path_end >= dir_path) && (*path_end == '/')) {
    *path_end = '\0';
    path_end--;
  }
  return file_list_build(dir_path);
}

void file_list_free(void) {
  list_free(file_list);
}

void *file_list_get(void) {
  return file_list;
}
