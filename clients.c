/* Herve Fache

20061023 Creation
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
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

static list_t clients  = NULL;
static char   *mounted = NULL;

static int unmount_share(const char *mount_point) {
  if (mounted != NULL) {
    free(mounted);
    mounted = NULL;
    return umount(mount_point);
  }
  return 0;
}

static char *mount_share(const char *mount_point, const client_t *client) {
  int result = 0;

  if (! strcmp(client->protocol, "smb")) {
    /* Mount SaMBa share (//hostname/?$) */
    char *share = malloc(strlen(client->hostname) + 6);

    sprintf(share, "//%s/%c$", client->hostname, client->listfile[0]);
    if ((mounted != NULL) && strcmp(mounted, share)) {
      /* Different share mounted: unmount */
      unmount_share(mount_point);
    }
    if (mounted != NULL) {
      /* Same share mounted */
      free(share);
    } else {
      /* No share mounted */
      char *command;
      int  command_length = strlen("mount -t smbfs") + 1  /* Ending '\0' */
                          + strlen(mount_point) + 1       /* Space before */
                          + strlen(share) + 1;            /* Space before */

      if (client->username != NULL) {
        command_length += strlen(" -o username=") + strlen(client->username);
        if (client->password != NULL) {
          command_length += strlen(",password=") + strlen(client->username);
        }
      }

      command = malloc(command_length);
      strcpy(command, "mount -t smbfs");
      if (client->username != NULL) {
        strcat(command, " -o username=");
        strcat(command, client->username);
        if (client->password != NULL) {
          strcat(command, ",password=");
          strcat(command, client->username);
        }
      }
      strcat(command, " ");
      strcat(command, share);
      strcat(command, " ");
      strcat(command, mount_point);

      if (system(command)) {
        fprintf(stderr, "clients: backup: could not mount %s share\n",
          client->protocol);
        free(share);
        result = 1;
      } else {
        mounted = share;
      }
      free(command);
    }
    if (result == 0) {
      return &client->listfile[3];
    }
  } else {
    fprintf(stderr, "clients: backup: %s protocol not supported\n",
      client->protocol);
  }
  return NULL;
}

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

int clients_backup(const char *mount_point) {
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
    } else if ((listfilename = mount_share(mount_point, client)) == NULL) {
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
  unmount_share(mount_point);
  return failed;
}
