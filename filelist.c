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

static void filelist_data_get(const void *payload, char *string) {
  const filelist_data_t *filelist_data = payload;

  sprintf(string, "%s", filelist_data->path);
}

static filelist_data_t * filelist_entry_new(const filelist_data_t *filelist_data) {
  filelist_data_t *filelist_data_p = malloc(sizeof(filelist_data_t));
  *filelist_data_p = *filelist_data;
  return filelist_data_p;
}

static int iterate_directory(const char *path, parser_t *parser) {
  void *handle;
  filelist_data_t filelist_data;
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
    /* Ignore . and .. */
    if (! strcmp(dir_entry->d_name, ".") || ! strcmp(dir_entry->d_name, "..")) {
      continue;
    }
    strcpy(filelist_data.path, path);
    strcat(filelist_data.path, "/");
    strcat(filelist_data.path, dir_entry->d_name);
    if (metadata_get(filelist_data.path, &filelist_data.metadata)) {
      fprintf(stderr, "filelist: cannot get metadata: %s\n", filelist_data.path);
      return 2;
    }
    /* Let the parser analyse the file data to know whether to back it up */
    if ((parser != NULL) && parser->file_check(handle, &filelist_data)) {
      continue;
    }
    /* Now pass it through the filters */
    if ((filters_handle != NULL) && ! filters_match(filters_handle, filelist_data.path)) {
      continue;
    }
    if (S_ISDIR(filelist_data.metadata.type)) {
      if (iterate_directory(filelist_data.path, parser)) {
        fprintf(stderr, "filelist: cannot iterate into directory: %s\n", dir_entry->d_name);
        return 2;
      }
    }
    list_add(file_list, filelist_entry_new(&filelist_data));
  }
  closedir(directory);
  return 0;
}

/*static int filelist_build(const char *path) {
  metadata_t filelist_data;

  if (metadata_get(path, &filelist_data)) {
    fprintf(stderr, "filelist: build: cannot get metadata: %s\n", path);
    return 2;
  }
  list_add(file_list, filelist_entry_new(&filelist_data));
  return iterate_directory(path, NULL);
}
*/
int filelist_new(const char *path, void *filters) {
  char dir_path[FILENAME_MAX];
  char *path_end = dir_path;

  filters_handle = filters;
  file_list = list_new(filelist_data_get);
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
  return iterate_directory(dir_path, NULL);
}

void filelist_free(void) {
  list_free(file_list);
}

void *filelist_get(void) {
  return file_list;
}
