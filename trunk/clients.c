/* Herve Fache

20061023 Creation
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "params.h"
#include "list.h"
#include "filters.h"
#include "parsers.h"
#include "filelist.h"
#include "cvs_parser.h"
#include "db.h"
#include "hbackup.h"
#include "clients.h"

typedef struct {
  char    *protocol;
  char    *username;
  char    *password;
  char    *hostname;
  char    *listfile;
} client_t;

typedef struct {
  char    *path;
  list_t  compress_handle;
  list_t  ignore_handle;
  list_t  parsers_handle;
} backup_t;

static list_t clients = NULL;

int clients_new(void) {
  /* Create new list */
  clients = list_new(NULL);
  if (clients == NULL) {
    fprintf(stderr, "clients: new: cannot intialise\n");
    return 2;
  }
  return 0;
}

void clients_free(void) {
  list_entry_t entry = NULL;

  while ((entry = list_next(clients, entry)) != NULL) {
    client_t *client = list_entry_payload(entry);

    free(client->protocol);
    free(client->username);
    free(client->password);
    free(client->hostname);
    free(client->listfile);
  }
  list_free(clients);
}

int clients_add(const char *info, const char *listfile) {
  client_t *client = malloc(sizeof(client_t));
  const char *start = info;
  char *delim = strchr(start, ':');

  if (delim == NULL) {
    return 1;
  }
  /* Protocol */
  client->protocol = malloc(delim - start + 1);
  strncpy(client->protocol, start, delim - start);
  client->protocol[delim - start] = '\0';
  /* Lower case */
  strtolower(client->protocol);
  start = delim + 1;
  /* There should be two / now */
  if (*start != '/') {
    return 1;
  }
  start++;
  if (*start != '/') {
    return 1;
  }
  start++;
  /* Host name */
  delim = strchr(start, '@');
  if (delim == NULL) {
    /* No username/password */
    client->username = NULL;
    client->password = NULL;
    client->hostname = malloc(strlen(start) + 1);
    strcpy(client->hostname, start);
    /* Lower case */
    strtolower(client->hostname);
  } else {
    char *colon = strchr(start, ':');

    client->hostname = malloc(strlen(delim + 1) + 1);
    strcpy(client->hostname, delim + 1);
    /* Lower case */
    strtolower(client->hostname);
    if (colon == NULL) {
      /* No password */
      client->username = malloc(delim - start + 1);
      strncpy(client->username, start, delim - start);
      client->username[delim - start] = '\0';
      client->password = NULL;
    } else {
      client->username = malloc(colon - start + 1);
      strncpy(client->username, start, colon - start);
      client->username[colon - start] = '\0';
      client->password = malloc(delim - colon - 1 + 1);
      strncpy(client->password, colon + 1, delim - colon - 1);
      client->password[delim - colon - 1] = '\0';
    }
  }

  /* List file */
  client->listfile = malloc(strlen(listfile) + 1);
  strcpy(client->listfile, listfile);
  if (! strcmp(client->protocol, "smb")) {
    /* Lower case */
    strtolower(client->listfile);
    pathtolinux(client->listfile);
  }

  list_append(clients, client);
  return 0;
}

static int add_filter(list_t handle, const char *type, const char *string) {
  if (! strcmp(type, "end")) {
    filters_add(handle, string, filter_end);
  } else if (! strcmp(type, "path_start")) {
    filters_add(handle, string, filter_path_start);
  } else if (! strcmp(type, "path_regexp")) {
    filters_add(handle, string, filter_path_regexp);
  } else if (! strcmp(type, "file_start")) {
    filters_add(handle, string, filter_file_start);
  } else if (! strcmp(type, "file_regexp")) {
    filters_add(handle, string, filter_file_regexp);
  } else {
    return 1;
  }
  return 0;
}

int clients_backup(void) {
  list_entry_t *entry = NULL;
  int failed = 0;

  /* Walk though the list of clients */
  while ((entry = list_next(clients, entry)) != NULL) {
    client_t *client = list_entry_payload(entry);
    FILE *listfile;
    char *listfilename;
    char *buffer = malloc(FILENAME_MAX);
    size_t size;
    int line = 0;
    list_t backups = list_new(NULL);
    backup_t *backup = NULL;

    /* Mount appropriate share to read backup list file */
    if (! strcmp(client->protocol, "file")) {
      /* Nothing to mount, file name is whatever was given */
      listfilename = client->listfile;
    } else {
      /* TODO mount share */
      fprintf(stderr, "clients: backup: unsupported protocol: %s, ignored\n",
        client->protocol);
      failed = 1;
      continue;
    }

    /* Open list file */
    if ((listfile = fopen(listfilename, "r")) == NULL) {
      fprintf(stderr, "clients: backup: list file not found %s\n",
        listfilename);
      failed = 2;
      continue;
    }
    /* Read list file */
    while (getline(&buffer, &size, listfile) >= 0) {
      char keyword[256];
      char type[256];
      char string[FILENAME_MAX];
      int params = params_readline(buffer, keyword, type, string);

      line++;
      if (params <= 0) {
        if (params < 0) {
          fprintf(stderr,
            "clients: backup: syntax error in list file %s, line %u\n",
            listfilename, line);
          failed = 1;
        }
        continue;
      }
      if (! strcmp(keyword, "compress")) {
        fprintf(stderr,
          "clients: backup: keyword not implemented in list file %s, line %u\n",
          listfilename, line);
        if (add_filter(backup->compress_handle, type, string)) {
          fprintf(stderr,
            "clients: backup: unsupported filter type in list file %s, line %u\n",
            listfilename, line);
        }
      } else if (! strcmp(keyword, "ignore")) {
        if (add_filter(backup->ignore_handle, type, string)) {
          fprintf(stderr,
            "clients: backup: unsupported filter type in list file %s, line %u\n",
            listfilename, line);
        }
      } else if (! strcmp(keyword, "parser")) {
        strtolower(string);
        if (! strcmp(string, "cvs")) {
          parsers_add(backup->parsers_handle, cvs_parser_new());
        } else {
          fprintf(stderr,
            "clients: backup: unsupported parser in list file %s, line %u\n",
            listfilename, line);
        }
      } else if (! strcmp(keyword, "path")) {
        /* New backup entry */
        backup = malloc(sizeof(backup_t));
        filters_new(&backup->compress_handle);
        filters_new(&backup->ignore_handle);
        parsers_new(&backup->parsers_handle);
        backup->path = malloc(strlen(string) + 2);
        strcpy(backup->path, string);
        one_trailing_slash(backup->path);
        list_append(backups, backup);
      } else {
        fprintf(stderr,
          "clients: backup: syntax error in list file %s, line %u\n",
          listfilename, line);
        failed = 1;
        continue;
      }
    }
    /* Close list file */
    fclose(listfile);

    /* Backup */
    if (verbosity() > 0) {
      printf("Backup client '%s' using protocol '%s'\n", client->hostname,
        client->protocol);
    }

    if (list_size(backups) == 0) {
      fprintf(stderr, "clients: backup: empty list!\n");
      failed = 1;
    } else {
      list_entry_t entry = NULL;

      while ((entry = list_next(backups, entry)) != NULL) {
        char prefix[FILENAME_MAX];
        backup_t *backup = list_entry_payload(entry);

        sprintf(prefix, "%s://%s", client->protocol, client->hostname);
        if (! failed) {
          if (verbosity() > 0) {
            printf("Backup path '%s'\n", backup->path);
            if (verbosity() > 1) {
              printf(" -> Building list of files\n");
            }
          }
          if (! filelist_new(backup->path, backup->ignore_handle,
            backup->parsers_handle)) {
            if (verbosity() > 1) {
              printf(" -> Parsing list of files\n");
            }
            if (db_parse(prefix, backup->path, filelist_getpath(),
              filelist_getlist())) {
              if (! terminating()) {
                fprintf(stderr, "clients: backup: parsing failed\n");
              }
              failed = 1;
            }
            filelist_free();
          } else {
            if (! terminating()) {
              fprintf(stderr, "clients: backup: list creation failed\n");
            }
            failed = 1;
          }
        }

        /* Free encapsulated data */
        free(backup->path);
        filters_free(backup->compress_handle);
        filters_free(backup->ignore_handle);
        parsers_free(backup->parsers_handle);
      }
    }
    /* Free backups list */
    list_free(backups);
  }

  return failed;
}
