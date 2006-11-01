/* Herve Fache

20061023 Creation
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include "params.h"
#include "tools.h"
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
  int failed = 0;

  if (mounted != NULL) {
    char *command = NULL;

    free(mounted);
    mounted = NULL;
    asprintf(&command, "umount %s", mount_point);
    if (verbosity() > 2) {
      printf(" --> Issuing command: %s\n", command);
    }
    if (system(command)) {
      failed = 1;
    }
    free(command);
  }
  return failed;
}

static int mount_share(const char *mount_point, const client_t *client,
    const char *client_share) {
  int result = 0;

  if (! strcmp(client->protocol, "smb")) {
    /* Mount SaMBa share (//hostname/share) */
    char *share = NULL;

    asprintf(&share, "//%s/%s", client->hostname, client_share);
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
      int  command_length = strlen("mount -t smbfs -o ro,nocase") + 1  /* Ending '\0' */
                          + strlen(mount_point) + 1       /* Space before */
                          + strlen(share) + 1;            /* Space before */

      if (client->username != NULL) {
        command_length += strlen(",username=") + strlen(client->username);
        if (client->password != NULL) {
          command_length += strlen(",password=") + strlen(client->password);
        }
      }

      command = malloc(command_length);
      strcpy(command, "mount -t smbfs -o ro,nocase");
      if (client->username != NULL) {
        strcat(command, ",username=");
        strcat(command, client->username);
        if (client->password != NULL) {
          strcat(command, ",password=");
          strcat(command, client->password);
        }
      }
      strcat(command, " ");
      strcat(command, share);
      strcat(command, " ");
      strcat(command, mount_point);

      if (verbosity() > 2) {
        printf(" --> Issuing command: %s\n", command);
      }
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
  }
  return result;
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
  char     *linecopy = NULL;
  char     *start;
  char     *delim;
  int      failed = 0;

  asprintf(&linecopy, "%s", info);
  start = linecopy;
  delim = strchr(start, ':');
  if (delim == NULL) {
    failed = 1;
  } else {
    /* Protocol */
    *delim = '\0';
    asprintf(&client->protocol, "%s", start);
    /* Lower case */
    strtolower(client->protocol);
    start = delim + 1;
    /* There should be two / now */
    if ((*start++ != '/') || (*start++ != '/')) {
      failed = 1;
    } else {
      /* Host name */
      delim = strchr(start, '@');
      if (delim == NULL) {
        /* No username/password */
        client->username = NULL;
        client->password = NULL;
        asprintf(&client->hostname, "%s", start);
        /* Lower case */
        strtolower(client->hostname);
      } else {
        char *colon = strchr(start, ':');

        *delim = '\0';
        /* Host name */
        asprintf(&client->hostname, "%s", delim + 1);
        strtolower(client->hostname);
        /* Password */
        if (colon == NULL) {
          /* No password */
          client->password = NULL;
        } else {
          if (strlen(colon + 1) != 0) {
            asprintf(&client->password, "%s", colon + 1);
          } else {
            client->password = NULL;
          }
          *colon = '\0';
        }
        /* Username */
        if (strlen(start) != 0) {
          asprintf(&client->username, "%s", start);
        } else {
          client->username = NULL;
        }
      }

      /* List file */
      asprintf(&client->listfile, "%s", listfile);
      list_append(clients, client);
    }
  }
  free(linecopy);
  return failed;
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

static int read_listfile(const char *listfilename, list_t backups) {
  FILE     *listfile;
  char     *buffer = NULL;
  size_t   size    = 0;
  int      line    = 0;
  backup_t *backup = NULL;
  int      failed  = 0;

  /* Open list file */
  if ((listfile = fopen(listfilename, "r")) == NULL) {
    fprintf(stderr, "clients: backup: list file not found %s\n",
      listfilename);
    failed = 2;
  } else {
    /* Read list file */
    while (getline(&buffer, &size, listfile) >= 0) {
      char keyword[256];
      char type[256];
      char *string = malloc(size);
      int params = params_readline(buffer, keyword, type, string);

      line++;
      if (params <= 0) {
        if (params < 0) {
          fprintf(stderr,
            "clients: backup: syntax error in list file %s, line %u\n",
            listfilename, line);
          failed = 1;
        }
      } else if (! strcmp(keyword, "compress")) {
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
        asprintf(&backup->path, "%s", string);
        no_trailing_slash(backup->path);
        list_append(backups, backup);
      } else {
        fprintf(stderr,
          "clients: backup: syntax error in list file %s, line %u\n",
          listfilename, line);
        failed = 1;
      }
      free(string);
    }
    /* Close list file */
    fclose(listfile);
  }
  free(buffer);
  return failed;
}

/* Return 1 to force mount */
static int get_paths(const char *protocol, const char *backup_path,
    const char *mount_point, char **share, char **path) {
  int status = 2;

  if (! strcmp(protocol, "file")) {
    *share = malloc(1);
    *share[0] = '\0';
    asprintf(path, "%s", backup_path);
    status = 0;
  } else if (! strcmp(protocol, "smb")) {
    asprintf(share, "%c$", backup_path[0]);
    asprintf(path, "%s/%s", mount_point, &backup_path[3]);
    /* Lower case */
    strtolower(*path);
    pathtolinux(*path);
    status = 1;
  }
  return status;
}

static int prepare_share(client_t *client, const char *mount_point,
    const char *backup_path, char **path) {
  char  *share = NULL;
  int   failed = 0;

  *path = NULL;
  failed = get_paths(client->protocol, backup_path, mount_point, &share, path);
  switch (failed) {
    case 1:
      if (! mount_share(mount_point, client, share)) {
        failed = 0;
      }
      break;
    case 2:
      fprintf(stderr, "clients: backup: %s protocol not supported\n",
        client->protocol);
  }
  free(share);
  return failed;
}

int clients_backup(const char *mount_point) {
  list_entry_t *entry = NULL;
  int failed = 0;

  /* Walk though the list of clients */
  while ((entry = list_next(clients, entry)) != NULL) {
    client_t *client       = list_entry_payload(entry);
    char     *listfilename = NULL;
    list_t   backups       = list_new(NULL);

    if (! prepare_share(client, mount_point, client->listfile, &listfilename)
     && ! read_listfile(listfilename, backups)) {
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
          backup_t *backup = list_entry_payload(entry);
          char     *backup_path = NULL;

          if (! failed) {
            if (verbosity() > 0) {
              printf("Backup path '%s'\n", backup->path);
              if (verbosity() > 1) {
                printf(" -> Building list of files\n");
              }
            }

            if (! prepare_share(client, mount_point, backup->path,
                    &backup_path)
             && ! filelist_new(backup_path, backup->ignore_handle,
                backup->parsers_handle)) {
              char *prefix = NULL;

              if (verbosity() > 1) {
                printf(" -> Parsing list of files\n");
              }
              asprintf(&prefix, "%s://%s", client->protocol, client->hostname);
              strtolower(backup->path);
              pathtolinux(backup->path);
              if (db_parse(prefix, backup->path, backup_path,
                  filelist_get())) {
                if (! terminating()) {
                  fprintf(stderr, "clients: backup: parsing failed\n");
                }
                failed = 1;
              }
              free(prefix);
              filelist_free();
            } else {
              if (! terminating()) {
                fprintf(stderr, "clients: backup: list creation failed\n");
              }
              failed = 1;
            }
            free(backup_path);
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
    } else {
      failed = 1;
    }
    free(listfilename);
  }
  unmount_share(mount_point);
  return failed;
}
