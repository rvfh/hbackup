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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

using namespace std;

typedef struct {
  char    *protocol;
  char    *username;
  char    *password;
  char    *hostname;
  char    *listfile;
} client_t;

typedef struct {
  char    *path;
  Filter  *ignore_handle;
  List    *parsers_handle;
} backup_t;

static List *clients = NULL;
static char   *mounted = NULL;

static int unmount_share(const char *mount_point) {
  int failed = 0;

  if (mounted != NULL) {
    char *command = NULL;

    free(mounted);
    mounted = NULL;
    asprintf(&command, "umount %s", mount_point);
    if (system(command)) {
      failed = 1;
    }
    free(command);
  }
  return failed;
}

static int mount_share(const char *mount_point, const client_t *client,
    const char *client_share) {
  int  result   = 0;
  char *share   = NULL;
  char *command = NULL;

  /* Determine share */
  if (! strcmp(client->protocol, "nfs")) {
    /* Mount Network File System share (hostname:share) */
    asprintf(&share, "%s:%s", client->hostname, client_share);
  } else
  if (! strcmp(client->protocol, "smb")) {
    /* Mount SaMBa share (//hostname/share) */
    asprintf(&share, "//%s/%s", client->hostname, client_share);
  }

  /* Check what is mounted */
  if (mounted != NULL) {
    if (strcmp(mounted, share)) {
      /* Different share mounted: unmount */
      unmount_share(mount_point);
    } else {
      /* Same share mounted: nothing to do */
      free(share);
      return 0;
    }
  }

  /* Build mount command */
  if (! strcmp(client->protocol, "nfs")) {
    asprintf(&command, "mount -t nfs -o ro,nolock %s %s", share, mount_point);
  } else
  if (! strcmp(client->protocol, "smb")) {
    char  *credentials  = NULL;

    if (client->username != NULL) {
      char *password = NULL;

      if (client->password != NULL) {
        asprintf(&password, ",password=%s", client->password);
      } else {
        asprintf(&password, "%s", "");
      }
      asprintf(&credentials, ",username=%s%s", client->username, password);
      free(password);
    } else {
      asprintf(&credentials, "%s", "");
    }
    asprintf(&command, "mount -t smbfs -o ro,nocase%s %s %s > /dev/null 2>&1", credentials,
      share, mount_point);
    free(credentials);
  }

  /* Issue mount command */
  result = system(command);
  if (result != 0) {
    free(share);
    fprintf(stderr, "clients: backup: could not mount %s share\n",
      client->protocol);
  } else {
    mounted = share;
  }
  free(command);
  return result;
}

int clients_new(void) {
  /* Create new list */
  clients = new List();
  if (clients == NULL) {
    fprintf(stderr, "clients: new: cannot intialise\n");
    return 2;
  }
  return 0;
}

void clients_free(void) {
  list_entry_t *entry = NULL;

  while ((entry = clients->next(entry)) != NULL) {
    client_t *client = (client_t *) (list_entry_payload(entry));

    free(client->protocol);
    free(client->username);
    free(client->password);
    free(client->hostname);
    free(client->listfile);
  }
  delete clients;
}

int clients_add(const char *info, const char *listfile) {
  client_t *client = new client_t;
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
      if (verbosity() > 2) {
        cout << " --> Client: " << client->hostname << "\n";
      }
      clients->append(client);
    }
  }
  free(linecopy);
  return failed;
}

static int add_filter(Filter *handle, const char *type, const char *string) {
  const char *filter_type;
  const char *delim    = strchr(type, '/');
  mode_t     file_type = 0;

  /* Check whether file type was specified */
  if (delim != NULL) {
    filter_type = delim + 1;
    if (! strncmp(type, "file", delim - type)) {
      file_type = S_IFREG;
    } else if (! strncmp(type, "dir", delim - type)) {
      file_type = S_IFDIR;
    } else if (! strncmp(type, "char", delim - type)) {
      file_type = S_IFCHR;
    } else if (! strncmp(type, "block", delim - type)) {
      file_type = S_IFBLK;
    } else if (! strncmp(type, "pipe", delim - type)) {
      file_type = S_IFIFO;
    } else if (! strncmp(type, "link", delim - type)) {
      file_type = S_IFLNK;
    } else if (! strncmp(type, "socket", delim - type)) {
      file_type = S_IFSOCK;
    } else {
      return 1;
    }
  } else {
    filter_type = type;
    file_type = S_IFMT;
  }

  /* Add specified filter */
  if (! strcmp(filter_type, "path_end")) {
    handle->addRule(new Rule(new Condition(file_type, filter_path_end, string)));
  } else if (! strcmp(filter_type, "path_start")) {
    handle->addRule(new Rule(new Condition(file_type, filter_path_start, string)));
  } else if (! strcmp(filter_type, "path_regexp")) {
    handle->addRule(new Rule(new Condition(file_type, filter_path_regexp, string)));
  } else if (! strcmp(filter_type, "size_below")) {
    handle->addRule(new Rule(new Condition(0, filter_size_below, strtoul(string, NULL, 10))));
  } else if (! strcmp(filter_type, "size_above")) {
    handle->addRule(new Rule(new Condition(0, filter_size_above, strtoul(string, NULL, 10))));
  } else {
    return 1;
  }
  return 0;
}

static int add_parser(List *handle, const char *type, const char *string) {
  parser_mode_t mode;

  /* Determine mode */
  if (! strcmp(type, "mod")) {
    mode = parser_modified;
  } else if (! strcmp(type, "mod+oth")) {
    mode = parser_modifiedandothers;
  } else if (! strcmp(type, "oth")) {
    mode = parser_others;
  } else {
    /* Default */
    mode = parser_controlled;
  }

  /* Add specified parser */
  if (! strcmp(string, "cvs")) {
    parsers_add(handle, mode, cvs_parser_new());
  } else {
    return 1;
  }
  return 0;
}

static int read_listfile(const char *listfilename, List *backups) {
  FILE     *listfile;
  char     *buffer = NULL;
  size_t   size    = 0;
  int      line    = 0;
  backup_t *backup = NULL;
  int      failed  = 0;

  /* Open list file */
  if (verbosity() > 1) {
    printf(" -> Reading backup list\n");
  }
  if ((listfile = fopen(listfilename, "r")) == NULL) {
    fprintf(stderr, "clients: backup: list file not found %s\n",
      listfilename);
    failed = 2;
  } else {
    /* Read list file */
    while (getline(&buffer, &size, listfile) >= 0) {
      char keyword[256];
      char type[256];
      char *string = new char[size];
      int params = params_readline(buffer, keyword, type, string);

      line++;
      if (params <= 0) {
        if (params < 0) {
          fprintf(stderr,
            "clients: backup: syntax error in list file %s, line %u\n",
            listfilename, line);
          failed = 1;
        }
      } else if (! strcmp(keyword, "ignore")) {
        if (add_filter(backup->ignore_handle, type, string)) {
          fprintf(stderr,
            "clients: backup: unsupported filter in list file %s, line %u\n",
            listfilename, line);
        }
      } else if (! strcmp(keyword, "parser")) {
        strtolower(string);
        if (add_parser(backup->parsers_handle, type, string)) {
          fprintf(stderr,
            "clients: backup: unsupported parser in list file %s, line %u\n",
            listfilename, line);
        }
      } else if (! strcmp(keyword, "path")) {
        /* New backup entry */
        backup = new backup_t;
        backup->ignore_handle = new Filter;
        parsers_new(&backup->parsers_handle);
        asprintf(&backup->path, "%s", string);
        if (verbosity() > 2) {
          printf(" --> Path: %s\n", string);
        }
        no_trailing_slash(backup->path);
        backups->append(backup);
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
    asprintf(share, "%s", "");
    asprintf(path, "%s", backup_path);
    status = 0;
  } else
  if (! strcmp(protocol, "smb")) {
    asprintf(share, "%c$", backup_path[0]);
    strtolower(*share);
    asprintf(path, "%s/%s", mount_point, &backup_path[3]);
    /* Lower case */
    strtolower(*path);
    pathtolinux(*path);
    status = 1;
  } else
  if (! strcmp(protocol, "nfs")) {
    asprintf(share, "%s", backup_path);
    asprintf(path, "%s", mount_point);
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

int clients_backup(const char *mount_point, int configcheck) {
  list_entry_t *entry = NULL;
  int failed = 0;

  /* Walk though the list of clients */
  while (((entry = clients->next(entry)) != NULL) && ! terminating()) {
    client_t  *client       = (client_t *) (list_entry_payload(entry));
    char      *listfilename = NULL;
    List      *backups      = new List();
    int       clientfailed  = 0;

    if (verbosity() > 0) {
      printf("Backup client '%s' using protocol '%s'\n", client->hostname,
        client->protocol);
    }

    if (! prepare_share(client, mount_point, client->listfile, &listfilename)
     && ! read_listfile(listfilename, backups)) {
      /* Backup */
      if (backups->size() == 0) {
        fprintf(stderr, "clients: backup: empty list!\n");
        failed = 1;
      } else if (! configcheck) {
        list_entry_t *entry = NULL;

        while (((entry = backups->next(entry)) != NULL) && ! clientfailed) {
          backup_t *backup = (backup_t *) (list_entry_payload(entry));
          char     *backup_path = NULL;

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
                filelist_get(), 100)) {
              if (! terminating()) {
                fprintf(stderr, "clients: backup: parsing failed\n");
              }
              failed        = 1;
            }
            free(prefix);
            filelist_free();
          } else {
            if (! terminating()) {
              fprintf(stderr, "clients: backup: list creation failed\n");
            }
            failed        = 1;
            clientfailed  = 1;
          }
          free(backup_path);

          /* Free encapsulated data */
          free(backup->path);
          delete backup->ignore_handle;
          parsers_free(backup->parsers_handle);
        }
      }
    } else {
      failed = 1;
    }
    /* Free backups list */
    delete backups;
    free(listfilename);
  }
  unmount_share(mount_point);
  return failed;
}
