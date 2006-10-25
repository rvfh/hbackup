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
#include "hbackup.h"
#include "list.h"
#include "metadata.h"
#include "filters.h"
#include "parsers.h"
#include "params.h"
#include "filelist.h"

/* Mount point */
static char mount_path[FILENAME_MAX] = "";
static int mount_path_length = 0;

static list_t files = NULL;
static void *filters_handle = NULL;
static void *parsers_handle = NULL;

static void filedata_get(const void *payload, char *string) {
  const filedata_t *filedata = payload;

  sprintf(string, "%s", filedata->path);
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
  while (((dir_entry = readdir(directory)) != NULL) && ! terminating()) {
    char *file_path;
    int  failed = 0;

    /* Ignore . and .. */
    if (! strcmp(dir_entry->d_name, ".") || ! strcmp(dir_entry->d_name, "..")){
      continue;
    }
    /* Add 2 for '\0' and '/' */
    file_path = malloc(strlen(path) + strlen(dir_entry->d_name) + 2);
    strcpy(file_path, path);
    strcat(file_path, dir_entry->d_name);
    /* Remove mount path from records */
    filedata.path = malloc(strlen(file_path) - mount_path_length + 1);
    strcpy(filedata.path, &file_path[mount_path_length]);
    strcpy(filedata.checksum, "");
    if (metadata_get(file_path, &filedata.metadata)) {
      fprintf(stderr, "filelist: cannot get metadata: %s\n", file_path);
      failed = 2;
    } else
    /* Let the parser analyse the file data to know whether to back it up */
    if ((parser != NULL) && parser->file_check(handle, &filedata)) {
      failed = 1;
    } else
    /* Now pass it through the filters */
    if ((filters_handle != NULL) && ! filters_match(filters_handle,
      filedata.path)) {
      failed = 1;
    } else
    if (S_ISDIR(filedata.metadata.type)) {
      filedata.metadata.size = 0;
      strcat(file_path, "/");
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
      filedata_p = malloc(sizeof(filedata_t));
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

int filelist_new(const char *path, list_t filters, list_t parsers) {
  filters_handle = filters;
  parsers_handle = parsers;
  files = list_new(filedata_get);
  if (files == NULL) {
    fprintf(stderr, "filelist: new: cannot initialise\n");
    return 2;
  }
  /* Remove trailing slashes */
  strcpy(mount_path, path);
  one_trailing_slash(mount_path);
  mount_path_length = strlen(mount_path);
  if (iterate_directory(mount_path, NULL)) {
    filelist_free();
    return 1;
  }
  return 0;
}

void filelist_free(void) {
  list_entry_t entry = NULL;

  while ((entry = list_next(files, entry)) != NULL) {
    filedata_t *filedata = list_entry_payload(entry);

    free(filedata->path);
  }
  list_free(files);
}

list_t filelist_getlist(void) {
  return files;
}

const char *filelist_getpath(void) {
  return mount_path;
}
