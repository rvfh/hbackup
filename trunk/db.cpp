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

/* Compression to use when required: gzip -5 (best speed/ratio) */

/* List file contents:
 *  host          (given in the format: 'protocol://host')
 *  path          (metadata)
 *  type          (metadata)
 *  size          (metadata)
 *  modified time (metadata)
 *  owner         (metadata)
 *  group         (metadata)
 *  permissions   (metadata)
 *  link          (readlink)
 *  checksum      (database)
 *  date in       (database)
 *  date out      (database)
 *  mark
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include "filelist.h"
#include "list.h"
#include "tools.h"
#include "hbackup.h"
#include "filters.h"
#include "db.h"
using namespace std;

#define CHUNK 10240

typedef struct {
  char        *host;
  filedata_t  filedata;
  char        *link;
  time_t      date_in;
  time_t      date_out;
} db_data_t;

static List   *db_list = NULL;
static char   *db_path = NULL;

/* Size of path being backed up */
static int    backup_path_length = 0;

static int db_load(const char *filename, List *list) {
  char *source_path;
  FILE *readfile;
  int  failed = 0;

  asprintf(&source_path, "%s/%s", db_path, filename);
  if ((readfile = fopen(source_path, "r")) != NULL) {
    /* Read the file into memory */
    char *buffer = NULL;
    size_t size  = 0;

    while (getline(&buffer, &size, readfile) >= 0) {
      db_data_t db_data;
      db_data_t *db_data_p = NULL;
      char      *start = buffer;
      char      *delim;
      char      *string = new char[size];
      char      letter;
      int       field = 0;
      int       failed = 1;

      while ((delim = strchr(start, '\t')) != NULL) {
        /* Get string portion */
        strncpy(string, start, delim - start);
        string[delim - start] = '\0';
        /* Extract data */
        failed = 0;
        switch (++field) {
          case 1:   /* Prefix */
            asprintf(&db_data.host, "%s", string);
            break;
          case 2:   /* Path */
            asprintf(&db_data.filedata.path, "%s", string);
            break;
          case 3:   /* Type */
            if (sscanf(string, "%c", &letter) != 1) {
              failed = 2;
            }
            db_data.filedata.metadata.type = type_mode(letter);
            break;
          case 4:   /* Size */
            if (sscanf(string, "%ld", &db_data.filedata.metadata.size) != 1) {
              failed = 2;
            }
            break;
          case 5:   /* Modification time */
            if (sscanf(string, "%ld", &db_data.filedata.metadata.mtime) != 1) {
              failed = 2;
            }
            break;
          case 6:   /* User */
            if (sscanf(string, "%u", &db_data.filedata.metadata.uid) != 1) {
              failed = 2;
            }
            break;
          case 7:   /* Group */
            if (sscanf(string, "%u", &db_data.filedata.metadata.gid) != 1) {
              failed = 2;
            }
            break;
          case 8:   /* Permissions */
            if (sscanf(string, "%o", &db_data.filedata.metadata.mode) != 1) {
              failed = 2;
            }
            break;
          case 9:   /* Link */
            asprintf(&db_data.link, "%s", string);
            break;
          case 10:  /* Checksum */
            strcpy(db_data.filedata.checksum, string);
            break;
          case 11:  /* Date in */
            if (sscanf(string, "%ld", &db_data.date_in) != 1) {
              failed = 2;
            }
            break;
          case 12:  /* Date out */
            if (sscanf(string, "%ld", &db_data.date_out) != 1) {
              failed = 2;
            }
            break;
          case 13:  /* Mark */
            if (strcmp(string, "-")) {
              failed = 2;
            }
            break;
          default:
            failed = 2;
        }
        start = delim + 1;
        if (failed) {
          if (field >= 1) {
            free(db_data.host);
          }
          if (field >= 9) {
            free(db_data.link);
          }
          break;
        }
      }
      free(string);
      if (failed) {
        fprintf(stderr, "db: failed to read list file (%s): wrong format (%d)\n", source_path, field);
      } else {
        db_data_p = new db_data_t;
        *db_data_p = db_data;
        list->add(db_data_p);
      }
    }
    free(buffer);
    fclose(readfile);
  } else {
    failed = 1;
  }
  free(source_path);
  return failed;
}

static int db_save(const char *filename, List *list) {
  char *temp_path;
  FILE *writefile;
  int  failed = 0;

  asprintf(&temp_path, "%s/%s.part", db_path, filename);
  if ((writefile = fopen(temp_path, "w")) != NULL) {
    char *dest_path = NULL;
    list_entry_t *entry = NULL;

    while ((entry = list->next(entry)) != NULL) {
      db_data_t *db_data = (db_data_t *) (list_entry_payload(entry));
      char *link = "";

      /* The link could also be stored as data... */
      if (db_data->link != NULL) {
        link = db_data->link;
      }
      fprintf(writefile,
        "%s\t%s\t%c\t%ld\t%ld\t%u\t%u\t%o\t%s\t%s\t%ld\t%ld\t%c\n",
        db_data->host, db_data->filedata.path,
        type_letter(db_data->filedata.metadata.type),
        db_data->filedata.metadata.size, db_data->filedata.metadata.mtime,
        db_data->filedata.metadata.uid, db_data->filedata.metadata.gid,
        db_data->filedata.metadata.mode, link, db_data->filedata.checksum,
        db_data->date_in, db_data->date_out, '-');
    }
    fclose(writefile);

    /* All done */
    asprintf(&dest_path, "%s/%s", db_path, filename);
    failed = rename(temp_path, dest_path);
    free(dest_path);
  } else {
    fprintf(stderr, "db: failed to write list file: %s\n", filename);
    failed = 2;
  }
  free(temp_path);
  return failed;
}

static char *db_data_get(const void *payload) {
  const db_data_t *db_data = (const db_data_t *) (payload);
  char *string = NULL;

  if (db_data->date_out == 0) {
    /* '@' > '9' */
    asprintf(&string, "%s %s %c", db_data->host, db_data->filedata.path, '@');
  } else {
    /* ' ' > '0' */
    asprintf(&string, "%s %s %11ld", db_data->host, db_data->filedata.path,
      db_data->date_out);
  }
  return string;
}

static int db_organize(char *path, int number) {
  DIR           *directory;
  struct dirent *dir_entry;
  char          *nofiles = NULL;
  int           failed   = 0;

  /* Already organized? */
  asprintf(&nofiles, "%s/.nofiles", path);
  if (! testfile(nofiles, 0)) {
    free(nofiles);
    return 0;
  }
  /* Find out how many entries */
  if ((directory = opendir(path)) == NULL) {
    free(nofiles);
    return 1;
  }
  while (((dir_entry = readdir(directory)) != NULL) && (number > 0)) {
    number--;
  }
  /* Decide what to do */
  if (number == 0) {
    if (verbosity() > 2) {
      printf(" --> Reorganizing db in: '%s' ...", path);
    }
    rewinddir(directory);
    while ((dir_entry = readdir(directory)) != NULL) {
      metadata_t  metadata;
      char        *source_path;

      /* Ignore . and .. */
      if (! strcmp(dir_entry->d_name, ".")
       || ! strcmp(dir_entry->d_name, "..")) {
        continue;
      }
      asprintf(&source_path, "%s/%s", path, dir_entry->d_name);
      if (metadata_get(source_path, &metadata)) {
        fprintf(stderr, "db: organize: cannot get metadata: %s\n", source_path);
        failed = 2;
      } else if (S_ISDIR(metadata.type) && (dir_entry->d_name[2] != '\0')
        /* If we crashed, we might have some two-letter dirs already */
              && (dir_entry->d_name[2] != '-')) {
        /* If we've reached the point where the dir is ??-?, stop! */
        char *dir_path = NULL;

        /* Create two-letter directory */
        asprintf(&dir_path, "%s/%c%c", path, dir_entry->d_name[0],
          dir_entry->d_name[1]);
        if (testdir(dir_path, 1) == 2) {
          failed = 2;
        } else {
          char *dest_path = NULL;

          /* Create destination path */
          asprintf(&dest_path, "%s/%s", dir_path, &dir_entry->d_name[2]);
          /* Move directory accross, changing its name */
          if (rename(source_path, dest_path)) {
            failed = 1;
          }
          free(dest_path);
        }
        free(dir_path);
      }
      free(source_path);
    }
    if (! failed) {
      testfile(nofiles, 1);
    }
    if (verbosity() > 2) {
      printf(" done\n");
    }
  }
  closedir(directory);
  free(nofiles);
  return failed;
}

static void db_list_free(List *list) {
  list_entry_t *entry = NULL;

  while ((entry = list->next(entry)) != NULL) {
    db_data_t *db_data = (db_data_t *) (list_entry_payload(entry));

    free(db_data->host);
    free(db_data->link);
    free(db_data->filedata.path);
  }
  delete list;
}

static int db_write(const char *mount_path, const char *path,
    const db_data_t *db_data, char *checksum, int compress) {
  char    *source_path = NULL;
  char    *temp_path   = NULL;
  char    *dest_path   = NULL;
  char    checksum_source[36];
  char    checksum_dest[36];
  FILE    *writefile;
  FILE    *readfile;
  int     index = 0;
  int     deleteit = 0;
  int     failed = 0;
  off_t   size_source;
  off_t   size_dest;

  /* File to read from */
  asprintf(&source_path, "%s/%s", mount_path, path);

  /* Temporary file to write to */
  asprintf(&temp_path, "%s/filedata", db_path);

  /* Copy file locally */
  if (zcopy(source_path, temp_path, &size_source, &size_dest,
      checksum_source, checksum_dest, compress)) {
    failed = 1;
  }
  free(source_path);

  /* Check size_source size */
  if (size_source != db_data->filedata.metadata.size) {
    fprintf(stderr, "db: write: file copy incomplete: %s\n", path);
    failed = 1;
  }

  /* Get file final location */
  if (! failed && getdir(db_path, checksum_source, &dest_path) == 2) {
    fprintf(stderr, "db: write: failed to get dir for: %s\n",
      checksum_source);
    failed = 1;
  }

  /* Make sure our checksum is unique */
  if (! failed) {
    do {
      char  *final_path = NULL;
      int   differ = 0;

      /* Complete checksum with index */
      sprintf((char *) checksum, "%s-%u", checksum_source, index);
      asprintf(&final_path, "%s-%u", dest_path, index);
      if (! testdir(final_path, 1)) {
        /* Directory exists */
        char  *try_path = NULL;

        asprintf(&try_path, "%s/data", final_path);
        if (! testfile(try_path, 0)) {
          /* A file already exists, let's compare */
          metadata_t try_md;
          metadata_t temp_md;

          differ = 1;

          /* Compare sizes first */
          if (! metadata_get(try_path, &try_md)
          && ! metadata_get(temp_path, &temp_md)
          && (try_md.size == temp_md.size)) {
            /* Compare try_path to temp_path */
            if ((readfile = fopen(try_path, "r")) != NULL) {
              if ((writefile = fopen(temp_path, "r")) != NULL) {
                char    buffer1[CHUNK];
                char    buffer2[CHUNK];
                differ = 0;

                do {
                  size_t length = fread(buffer1, 1, CHUNK, readfile);

                  /* Does fread read as much as possible? */
                  if ((fread(buffer2, 1, length, writefile) != length)
                   || strncmp(buffer1, buffer2, length)) {
                    differ = 1;
                    break;
                  }
                } while (! feof(readfile) && ! feof(writefile));
                /* Is there more data in one of the files? */
                if (!differ) {
                  differ = fread(buffer1, 1, 1, readfile)
                        || fread(buffer2, 1, 1, writefile);
                }
                if (! differ) {
                  deleteit = 1;
                }
                fclose(writefile);
              }
              fclose(readfile);
            }
          }
        }
        free(try_path);
      }
      if (! differ) {
        free(dest_path);
        dest_path = final_path;
        break;
      } else {
        free(final_path);
      }
      index++;
    } while (1);
  }

  /* Now move the file in its place */
  {
    char *data_path = NULL;

    asprintf(&data_path, "%s/data", dest_path);
    free(dest_path);
    dest_path = data_path;
  }
  if (! failed && rename(temp_path, dest_path)) {
    fprintf(stderr, "db: write: failed to move file %s to %s: %s\n",
      strerror(errno), temp_path, dest_path);
    failed = 1;
  }

  /* If anything failed, delete temporary file */
  if (failed || deleteit) {
    remove(temp_path);
  }
  free(temp_path);

  /* Save redundant information together with data */
  if (! failed) {
    /* dest_path is /path/to/checksum/data */
    char *delim = strrchr(dest_path, '/');

    if (delim != NULL) {
      char         *listfile = NULL;
      List       *list;
      list_entry_t *entry;
      db_data_t    new_db_data = *db_data;

      *delim = '\0';
      /* Now dest_path is /path/to/checksum */
      asprintf(&listfile, "%s/%s", &dest_path[strlen(db_path) + 1], "list");
      list = new List(db_data_get);
      if (db_load(listfile, list) == 2) {
        fprintf(stderr, "db: write: failed to open data list\n");
      } else {
        strcpy(new_db_data.filedata.checksum, checksum_dest);
        entry = list->add(&new_db_data);
        db_save(listfile, list);
        list->remove(entry);
      }
      db_list_free(list);
      free(listfile);
    }
  }

  /* Make sure we won't exceed the file number limit */
  if (! failed) {
    /* dest_path is /path/to/checksum */
    char *delim = strrchr(dest_path, '/');

    if (delim != NULL) {
      *delim = '\0';
      /* Now dest_path is /path/to */
      db_organize(dest_path, 256);
    }
  }
  free(dest_path);
  return failed;
}

static int db_obsolete(const char *prefix, const char *path,
    const char *checksum) {
  char         *listfile  = NULL;
  char         *temp_path = NULL;
  int          failed = 0;
  List       *list;

  if (getdir(db_path, checksum, &temp_path)) {
    fprintf(stderr, "db: obsolete: failed to get dir for: %s\n", checksum);
    return 2;
  }
  asprintf(&listfile, "%s/list", &temp_path[strlen(db_path) + 1]);
  free(temp_path);

  list = new List(db_data_get);
  if (db_load(listfile, list) == 2) {
    fprintf(stderr, "db: obsolete: failed to open data list\n");
  } else {
    db_data_t    *db_data;
    list_entry_t *entry;
    char         *string = NULL;

    asprintf(&string, "%s %s %c", prefix, path, '@');
    list->find(string, NULL, &entry);
    if (entry != NULL) {
      db_data = (db_data_t *) (list_entry_payload(entry));
      db_data->date_out = time(NULL);
      db_save(listfile, list);
    }
    free(string);
  }
  db_list_free(list);


  free(listfile);
  return failed;
}

static int db_lock(const char *path) {
  char *lock_path = NULL;
  FILE *file;
  int  status = 0;

  /* Set the database path that we just locked as default */
  asprintf(&db_path, "%s", path);

  asprintf(&lock_path, "%s/lock", path);

  /* Try to open lock file for reading: check who's holding the lock */
  if ((file = fopen(lock_path, "r")) != NULL) {
    pid_t pid;

    /* Lock already taken */
    fscanf(file, "%d", &pid);
    fclose(file);
    if (pid != 0) {
      /* Find out whether process is still running, if not, reset lock */
      kill(pid, 0);
      if (errno == ESRCH) {
        fprintf(stderr, "db: lock: lock reset\n");
        remove(lock_path);
      } else {
        fprintf(stderr, "db: lock: lock taken by process with pid %d\n", pid);
        status = 2;
      }
    } else {
      fprintf(stderr, "db: lock: lock taken by an unidentified process!\n");
      status = 2;
    }
  }

  /* Try to open lock file for writing: lock */
  if (! status) {
    if ((file = fopen(lock_path, "w")) != NULL) {
      /* Lock taken */
      fprintf(file, "%u\n", getpid());
      fclose(file);
    } else {
      /* Lock cannot be taken */
      fprintf(stderr, "db: lock: cannot take lock\n");
      status = 2;
    }
  }
  free(lock_path);
  return status;
}

static void db_unlock(const char *path) {
  char *lock_path = NULL;

  asprintf(&lock_path, "%s/lock", db_path);
  free(db_path);
  db_path = NULL;
  remove(lock_path);
  free(lock_path);
}


int db_open(const char *path) {
  int status;

  /* Take lock */
  if (db_lock(path)) {
    return 2;
  }

  /* Check that data dir exists, if not create it */
  {
    char *data_path = NULL;

    asprintf(&data_path, "%s/data", db_path);
    status = testdir(data_path, 1);
    free(data_path);
  }

  /* Read database active items list */
  if (status != 2) {
    db_list = new List(db_data_get);
    switch (db_load("list", db_list)) {
      case 0:
        if (verbosity() > 0) {
          printf("Database opened (active contents: %u file(s))\n",
            db_list->size());
        }
        break;
      case 1:
        if (! status) {
          /* TODO There was a data directory, but no list. Attempt recovery */
        } else if (verbosity() > 0) {
          printf("Database initialized\n");
        }
        break;
      default:
        status = 2;
    }
  }

  if (status == 2) {
    db_unlock(path);
    return 2;
  }
  return 0;
}

static char *close_select(const void *payload) {
  const db_data_t *db_data = (const db_data_t *) (payload);
  char *string = NULL;

  if (db_data->date_out == 0) {
    asprintf(&string, "@");
  } else {
    asprintf(&string, "#");
  }
  return string;
}

void db_close(void) {
  List      *active_list = new List();
  List      *removed_list = new List();
  time_t    localtime;
  struct tm localtime_brokendown;

  /* Save list for month day */
  if ((time(&localtime) != -1)
   && (localtime_r(&localtime, &localtime_brokendown) != NULL)) {
    char *daily_list = NULL;

    asprintf(&daily_list, "list_%02u", localtime_brokendown.tm_mday);
    db_save(daily_list, db_list);
    free(daily_list);
  }

  /* Also load previously removed items into list */
  /* TODO What do we do if this fails? Recover? */
  db_load("removed", db_list);

  /* Split list into active and removed records */
  db_list->select("@", close_select, active_list, removed_list);

  /* Save active list */
  if (db_save("list", active_list)) {
    fprintf(stderr, "db: close: failed to save active items list\n");
  }
  active_list->deselect();
  delete active_list;

  /* Save removed list */
  if (db_save("removed", removed_list)) {
    fprintf(stderr, "db: close: failed to save removed items list\n");
  }
  removed_list->deselect();
  delete removed_list;

  /* Release lock */
  db_unlock(db_path);
  if (verbosity() > 0) {
    printf("Database closed (total contents: %u file(s))\n",
      db_list->size());
  }
  db_list_free(db_list);
}

static char *parse_select(const void *payload) {
  const db_data_t *db_data = (const db_data_t *) (payload);
  char *string = NULL;

  if (db_data->date_out != 0) {
    /* This string cannot be matched */
    asprintf(&string, "\t");
  } else {
    asprintf(&string, "%s", db_data->filedata.path);
  }
  return string;
}

/* Need to compare only for matching paths */
static int parse_compare(void *db_data_p, void *filedata_p) {
  const db_data_t *db_data  = (const db_data_t *) (db_data_p);
  filedata_t      *filedata = (filedata_t *) (filedata_p);
  int             result;

  /* If paths differ, that's all we want to check */
  if ((result = strcmp(&db_data->filedata.path[backup_path_length],
      filedata->path))) {
    return result;
  }
  /* If the file has been modified, just return 1 or -1 and that should do */
  result = memcmp(&db_data->filedata.metadata, &filedata->metadata,
    sizeof(metadata_t));

  /* If it's a file and size and mtime are the same, copy checksum accross */
  if (S_ISREG(db_data->filedata.metadata.type)) {
    if (db_data->filedata.checksum[0] == 'N') {
      /* Checksum missing, add to added list */
      return 1;
    } else
    if ((db_data->filedata.metadata.size == filedata->metadata.size)
     || (db_data->filedata.metadata.mtime == filedata->metadata.mtime)) {
      strcpy(filedata->checksum, db_data->filedata.checksum);
    }
  }
  return result;
}

int db_parse(const char *host, const char *real_path,
    const char *mount_path, List *file_list, off_t compress_min) {
  List          *selected_files_list = new List(db_data_get);
  List          *added_files_list = new List();
  List          *removed_files_list = new List();
  list_entry_t  *entry   = NULL;
  char          *string = NULL;
  int           failed  = 0;

  /* Compare list with db list for matching host */
  asprintf(&string, "%s/", real_path);
  backup_path_length = strlen(string);
  db_list->select(string, parse_select, selected_files_list, NULL);
  free(string);
  selected_files_list->compare(file_list, added_files_list,
    removed_files_list, parse_compare);
  selected_files_list->deselect();

  /* Deal with new/modified data first */
  if (added_files_list->size() != 0) {
    /* Static to be global to all shares */
    static int    copied        = 0;
    static off_t  volume        = 0;
    off_t         sizetobackup  = 0;
    off_t         sizebackedup  = 0;
    /* Percents display stuff */
    off_t         step          = 0;
    off_t         nextstep      = 0;

    /* Determine volume to be copied */
    if (verbosity() > 2) {
      while ((entry = added_files_list->next(entry)) != NULL) {
        filedata_t *filedata = (filedata_t *) (list_entry_payload(entry));

        if (S_ISREG(filedata->metadata.type)) {
          sizetobackup += filedata->metadata.size;
        }
      }
      if (sizetobackup >= 100000) {
        nextstep = step = sizetobackup / 100;
      } else {
        nextstep = step = 0;
      }
      printf(" --> Files to add: %u (%lu bytes)\n",
        added_files_list->size(), sizetobackup);
    }

    while ((entry = added_files_list->next(entry)) != NULL) {
      if (! terminating()) {
        filedata_t *filedata = (filedata_t *) (list_entry_payload(entry));
        db_data_t  *db_data  = new db_data_t;

        asprintf(&db_data->host, "%s", host);
        db_data->filedata.metadata = filedata->metadata;
        asprintf(&db_data->filedata.path, "%s/%s", real_path,
          filedata->path);
        db_data->link = NULL;
        db_data->date_in = time(NULL);
        db_data->date_out = 0;
        /* Save new data */
        if (S_ISREG(filedata->metadata.type)) {
          if (verbosity() > 2) {
            sizebackedup += filedata->metadata.size;
          }
          if (filedata->checksum[0] != '\0') {
            /* Checksum given by the compare function */
            strcpy(db_data->filedata.checksum, filedata->checksum);
          } else {
            if (db_write(mount_path, filedata->path, db_data,
                db_data->filedata.checksum, 0)) {
              /* Write failed, need to go on */
              failed = 1;
              if (! terminating()) {
                /* Don't signal errors on termination */
                fprintf(stderr, "db: parse: %s: %s, ignoring\n",
                    strerror(errno), db_data->filedata.path);
                if ((verbosity() > 2) && (step != 0)) {
                  sizebackedup -= filedata->metadata.size;
                  sizetobackup -= filedata->metadata.size;
                  step          = sizetobackup / 100;
                  nextstep      = sizebackedup + step;
                }
              }
              strcpy(db_data->filedata.checksum, "N");
            }
          }
        } else {
          strcpy(db_data->filedata.checksum, "");
        }
        if (S_ISLNK(filedata->metadata.type)) {
          char *full_path = NULL;
          char *string = new char[FILENAME_MAX];
          int size;

          asprintf(&full_path, "%s/%s", mount_path, filedata->path);
          if ((size = readlink(full_path, string, FILENAME_MAX)) < 0) {
            failed = 1;
            fprintf(stderr, "db: parse: %s: %s, ignoring\n",
              strerror(errno), db_data->filedata.path);
          } else {
            string[size] = '\0';
            asprintf(&db_data->link, "%s", string);
          }
          free(full_path);
          free(string);
        }
        db_list->add(db_data);
        if ((++copied >= 1000)
         || ((volume += db_data->filedata.metadata.size) >= 10000000)) {
          copied = 0;
          volume = 0;
          db_save("list", db_list);
        }
        if ((verbosity() > 2) && (step != 0) && (sizebackedup >= nextstep)) {
          off_t percents = sizebackedup / step;
          printf(" --> Copied: %lu / %lu (%lu%%)\n", sizebackedup,
            sizetobackup, percents);
          /* Align nextstep to percent (step 2) */
          nextstep = step * (percents + 1 );
        }
      }
    }
  } else if (verbosity() > 2) {
    cout << " --> No files to add\n";
  }

  /* This only unlists the data */
  added_files_list->deselect();
  delete added_files_list;

  /* Deal with removed/modified data */
  if (removed_files_list->size() != 0) {
    if (verbosity() > 2) {
      printf(" --> Files to remove: %u\n", removed_files_list->size());
    }
    while ((entry = removed_files_list->next(entry)) != NULL) {
      db_data_t *db_data = (db_data_t *) (list_entry_payload(entry));

      /* Same data as in db_list */
      db_data->date_out = time(NULL);

      /* Update local list */
      if (S_ISREG(db_data->filedata.metadata.type)
       && (db_data->filedata.checksum[0] != '\0')
       && (db_data->filedata.checksum[0] != 'N')) {
        db_obsolete(db_data->host, db_data->filedata.path,
          db_data->filedata.checksum);
      }
    }
  } else if (verbosity() > 2) {
    cout << " --> No files to remove\n";
  }
  /* This only unlists the data */
  removed_files_list->deselect();
  delete removed_files_list;

  /* Report errors */
  if (failed) {
    return 1;
  }
  return 0;
}

int db_read(const char *path, const char *checksum) {
  char *source_path = NULL;
  char *temp_path   = NULL;
  char temp_checksum[256];
  int  failed = 0;

  if (getdir(db_path, checksum, &temp_path)) {
    fprintf(stderr, "db: read: failed to get dir for: %s\n", checksum);
    return 2;
  }
  asprintf(&source_path, "%s/data", temp_path);
  free(temp_path);
  temp_path = NULL;

  /* Open temporary file to write to */
  asprintf(&temp_path, "%s.part", path);

  /* Copy file to temporary name (size not checked: checksum suffices) */
  if (zcopy(source_path, temp_path, NULL, NULL, temp_checksum, NULL, 0)) {
    fprintf(stderr, "db: read: failed to copy file: %s\n", source_path);
    failed = 2;
  } else

  /* Verify that checksums match before overwriting current final destination */
  if (strncmp(checksum, temp_checksum, strlen(temp_checksum))) {
    fprintf(stderr, "db: read: checksums don't match: %s %s\n",
      source_path, temp_checksum);
    failed = 2;
  } else

  /* All done */
  if (rename(temp_path, path)) {
    fprintf(stderr, "db: read: failed to rename file to %s: %s\n",
      strerror(errno), path);
    failed = 2;
  }

  if (failed) {
    remove(temp_path);
  }
  free(source_path);
  free(temp_path);

  return failed;
}

int db_scan(const char *local_db_path, const char *checksum) {
  int failed = 0;

  if (local_db_path != NULL) {
    if (db_lock(local_db_path)) {
      fprintf(stderr, "db: scan: failed to lock\n");
      return 2;
    }
    db_list = new List(db_data_get);
    failed = db_load("list", db_list) | db_load("removed", db_list);
    if (failed) {
      fprintf(stderr, "db: scan: failed to read lists\n");
      failed = 2;
    }
  }
  if (! failed) {
    if (checksum == NULL) {
      list_entry_t *entry = NULL;
      int          files = db_list->size();

      if (verbosity() > 0) {
        printf("Scanning database (contents: %u file(s))\n", files);
      }
      while ((entry = db_list->next(entry)) != NULL) {
        db_data_t *db_data = (db_data_t *) (list_entry_payload(entry));

        if ((verbosity() > 2) && ((files & 0xFF) == 0)) {
          printf(" --> Files left to go: %u\n", files);
        }
        files--;
        if (db_scan(NULL, db_data->filedata.checksum)) {
          failed = 1;
          fprintf(stderr, "File data missing for checksum %s\n",
            db_data->filedata.checksum);
          if (! terminating() && verbosity() > 1) {
            struct tm *time;
            printf(" -> Client:      %s\n", db_data->host);
            printf(" -> File name:   %s\n", db_data->filedata.path);
            time = localtime(&db_data->filedata.metadata.mtime);
            printf(" -> Modified:    %04u-%02u-%02u %2u:%02u:%02u\n",
              time->tm_year + 1900, time->tm_mon + 1, time->tm_mday,
              time->tm_hour, time->tm_min, time->tm_sec);
            if (verbosity() > 2) {
              time = localtime(&db_data->date_in);
              printf(" --> Seen first: %04u-%02u-%02u %2u:%02u:%02u\n",
                time->tm_year + 1900, time->tm_mon + 1, time->tm_mday,
                time->tm_hour, time->tm_min, time->tm_sec);
              if (db_data->date_out != 0) {
                time = localtime(&db_data->date_out);
                printf(" --> Seen gone:  %04u-%02u-%02u %2u:%02u:%02u\n",
                  time->tm_year + 1900, time->tm_mon + 1, time->tm_mday,
                  time->tm_hour, time->tm_min, time->tm_sec);
              }
            }
          }
        }
        if (terminating()) {
          failed = 3;
          break;
        }
      }
    } else if ((checksum[0] != 'N') && (checksum[0] != '\0')) {
      char *path = NULL;

      if (getdir(db_path, checksum, &path)) {
        failed = 1;
      } else {
        char *test_path = NULL;

        asprintf(&test_path, "%s/data", path);
        if (testfile(test_path, 0)) {
          failed = 2;
        }
        free(test_path);
      }
      free(path);
    }
  }
  if (local_db_path != NULL) {
    db_list_free(db_list);
    db_unlock(local_db_path);
  }
  return failed;
}

int db_check(const char *local_db_path, const char *checksum) {
  int failed = 0;
  char checksum_real[256];

  if (local_db_path != NULL) {
    if (db_lock(local_db_path)) {
      fprintf(stderr, "db: check: failed to lock\n");
      return 2;
    }
    db_list = new List(db_data_get);
    failed = db_load("list", db_list) | db_load("removed", db_list);
    if (failed) {
      fprintf(stderr, "db: check: failed to read lists\n");
      failed = 2;
    }
  }
  if (! failed) {
    if (checksum == NULL) {
      list_entry_t *entry = NULL;
      int          files = db_list->size();

      if (verbosity() > 0) {
        printf("Checking database (contents: %u file(s))\n",
          db_list->size());
      }
      while ((entry = db_list->next(entry)) != NULL) {
        db_data_t *db_data = (db_data_t *) (list_entry_payload(entry));

        if ((verbosity() > 2) && ((files & 0xFF) == 0)) {
          printf(" --> Files left to go: %u\n", files);
        }
        files--;
        failed = db_check(NULL, db_data->filedata.checksum);
        if (terminating()) {
          failed = 3;
          break;
        }
      }
    } else if ((checksum[0] != 'N') && (checksum[0] != '\0')) {
      char       *path = NULL;

      if (getdir(db_path, checksum, &path)) {
        failed = 2;
      } else {
        char    *check_path = NULL;
        char    *listfile   = NULL;
        List  *list;

        asprintf(&listfile, "%s/list", &path[strlen(db_path) + 1]);
        list = new List(db_data_get);
        if (db_load(listfile, list) == 2) {
          fprintf(stderr, "db: check: failed to open data list\n");
        } else {
          list_entry_t *entry = list->next(NULL);

          if (entry == NULL) {
            fprintf(stderr, "db: check: failed to obtain checksum\n");
            failed = 2;
          } else {
            db_data_t *db_data = (db_data_t *) (list_entry_payload(entry));

            /* Read file to compute checksum, compare with expected */
            asprintf(&check_path, "%s/data", path);
            if (getchecksum(check_path, checksum_real) == 2) {
              fprintf(stderr, "File data missing for checksum %s\n", checksum);
              failed = 1;
            } else
            if (strncmp(db_data->filedata.checksum, checksum_real,
                  strlen(checksum_real)) != 0) {
              if (! terminating()) {
                fprintf(stderr,
                  "File data corrupted for checksum %s (found to be %s)\n",
                  checksum, checksum_real);
              }
              failed = 1;
            }
            free(check_path);

            if (! terminating() && (failed == 1) && verbosity() > 1) {
              struct tm *time;

              printf(" -> Client:      %s\n", db_data->host);
              printf(" -> File name:   %s\n", db_data->filedata.path);
              time = localtime(&db_data->filedata.metadata.mtime);
              printf(" -> Modified:    %04u-%02u-%02u %2u:%02u:%02u\n",
                time->tm_year + 1900, time->tm_mon + 1, time->tm_mday,
                time->tm_hour, time->tm_min, time->tm_sec);
              if (verbosity() > 2) {
                time = localtime(&db_data->date_in);
                printf(" --> Seen first: %04u-%02u-%02u %2u:%02u:%02u\n",
                  time->tm_year + 1900, time->tm_mon + 1, time->tm_mday,
                  time->tm_hour, time->tm_min, time->tm_sec);
                if (db_data->date_out != 0) {
                  time = localtime(&db_data->date_out);
                  printf(" --> Seen gone:  %04u-%02u-%02u %2u:%02u:%02u\n",
                    time->tm_year + 1900, time->tm_mon + 1, time->tm_mday,
                    time->tm_hour, time->tm_min, time->tm_sec);
                }
              }
            }
          }
        }
        db_list_free(list);
        free(listfile);
      }
      free(path);
    }
  }
  if (local_db_path != NULL) {
    db_list_free(db_list);
    db_unlock(local_db_path);
  }
  return failed;
}
