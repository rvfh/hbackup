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

/* Mount point */
static char mount_path[FILENAME_MAX] = "";
static int mount_path_length = 0;

static list_t file_list = NULL;
static void *filters_handle = NULL;
static void *parsers_handle = NULL;

static void filedata_get(const void *payload, char *string) {
  const filedata_t *filedata = payload;

  sprintf(string, "%s", filedata->path);
}

static filedata_t * filelist_entry_new(const filedata_t *filedata) {
  filedata_t *filedata_p = malloc(sizeof(filedata_t));
  *filedata_p = *filedata;
  return filedata_p;
}

static int iterate_directory(const char *path, parser_t *parser) {
  void *handle;
  filedata_t filedata;
  DIR *directory;
  struct dirent *dir_entry;

  /* Check whether directory is under SCM control */
  if (parser == NULL) {
    void *parser_handle = NULL;

    while ((parser = parsers_next(parsers_handle, &parser_handle)) != NULL) {
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
    char file_path[FILENAME_MAX];

    /* Ignore . and .. */
    if (! strcmp(dir_entry->d_name, ".") || ! strcmp(dir_entry->d_name, "..")){
      continue;
    }
    strcpy(file_path, path);
    strcat(file_path, dir_entry->d_name);
    /* Remove mount path from records */
    strcpy(filedata.path, &file_path[mount_path_length]);
    strcpy(filedata.checksum, "");
    if (metadata_get(file_path, &filedata.metadata)) {
      fprintf(stderr, "filelist: cannot get metadata: %s\n", file_path);
      return 2;
    }
    /* Let the parser analyse the file data to know whether to back it up */
    if ((parser != NULL) && parser->file_check(handle, &filedata)) {
      continue;
    }
    /* Now pass it through the filters */
    if ((filters_handle != NULL) && ! filters_match(filters_handle,
      filedata.path)) {
      continue;
    }
    if (S_ISDIR(filedata.metadata.type)) {
      filedata.metadata.size = 0;
      strcat(file_path, "/");
      if (iterate_directory(file_path, parser)) {
        fprintf(stderr, "filelist: cannot iterate into directory: %s\n",
          dir_entry->d_name);
        return 2;
      }
    }
    list_add(file_list, filelist_entry_new(&filedata));
  }
  closedir(directory);
  return 0;
}

int filelist_new(const char *path, list_t filters, list_t parsers) {
  char dir_path[FILENAME_MAX];
  char *path_end = dir_path;

  filters_handle = filters;
  parsers_handle = parsers;
  file_list = list_new(filedata_get);
  if (file_list == NULL) {
    fprintf(stderr, "filelist: new: cannot initialise\n");
    return 2;
  }
  /* Remove trailing slashes */
  strcpy(mount_path, path);
  path_end = strchr(mount_path, '/');
  if (path_end != NULL) {
    *path_end = '\0';
  }
  strcat(mount_path, "/");
  mount_path_length = strlen(mount_path);
  return iterate_directory(mount_path, NULL);
}

void filelist_free(void) {
  list_free(file_list);
}

list_t filelist_getlist(void) {
  return file_list;
}

const char *filelist_getpath(void) {
  return mount_path;
}
