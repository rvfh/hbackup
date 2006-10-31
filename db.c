/* Herve Fache

20061008 Creation
*/

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

#define _GNU_SOURCE
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <openssl/md5.h>
#include <openssl/evp.h>
#include "filelist.h"
#include "list.h"
#include "tools.h"
#include "hbackup.h"
#include "db.h"

typedef struct {
  char        *host;
  filedata_t  filedata;
  char        *link;
  time_t      date_in;
  time_t      date_out;
} db_data_t;

static list_t *db_list = NULL;
static char   *db_path = NULL;

/* Current path being backed up */
static char   *backup_path = NULL;
static int    backup_path_length = 0;

static char type_letter(mode_t mode) {
  if (S_ISREG(mode))  return 'f';
  if (S_ISDIR(mode))  return 'd';
  if (S_ISCHR(mode))  return 'c';
  if (S_ISBLK(mode))  return 'b';
  if (S_ISFIFO(mode)) return 'p';
  if (S_ISLNK(mode))  return 'l';
  if (S_ISSOCK(mode)) return 's';
  return '?';
}

static mode_t type_mode(char letter) {
  if (letter == 'f') return S_IFREG;
  if (letter == 'd') return S_IFDIR;
  if (letter == 'c') return S_IFCHR;
  if (letter == 'b') return S_IFBLK;
  if (letter == 'p') return S_IFIFO;
  if (letter == 'l') return S_IFLNK;
  if (letter == 's') return S_IFSOCK;
  return 0;
}

static void md5sum(char *checksum, int bytes) {
  char *hex            = "0123456789abcdef";
  unsigned char *copy  = malloc(bytes);
  unsigned char *read  = copy;
  unsigned char *write = (unsigned char *) checksum;

  memcpy(copy, checksum, bytes);
  while (bytes != 0) {
    *write++ = hex[*read >> 4];
    *write++ = hex[*read & 0xf];
    read++;
    bytes--;
  }
  *write = '\0';
  free(copy);
}

static size_t copy(const char *source_path, const char *dest_path, char *checksum) {
  FILE        *writefile;
  FILE        *readfile;
  char        buffer[10240];
  EVP_MD_CTX  ctx;
  size_t      rlength;
  size_t      wlength;
  size_t      size = 0;

  /* Open file to read from */
  if ((readfile = fopen(source_path, "r")) == NULL) {
    fprintf(stderr, "db: failed to open source file: %s\n", source_path);
    return -1;
  }

  /* Open file to write to */
  if ((writefile = fopen(dest_path, "w")) == NULL) {
    fclose(readfile);
    fprintf(stderr, "db: failed to open destination file: %s\n", dest_path);
    return -1;
  }

  /* We shall copy and compute the checksum in one go */
  EVP_DigestInit(&ctx, EVP_md5());
  while (! feof(readfile) && ! terminating()) {
    rlength = fread(buffer, 1, 10240, readfile);
    size += rlength;
    EVP_DigestUpdate(&ctx, buffer, rlength);
    do {
      wlength = fwrite(buffer, 1, rlength, writefile);
      rlength -= wlength;
    } while ((rlength != 0) && (wlength != 0));
  }
  fclose(readfile);
  fclose(writefile);
  /* Re-use length */
  EVP_DigestFinal(&ctx, (unsigned char *) checksum, &rlength);
  md5sum(checksum, rlength);
  return size;
}

static int getdir(const char *checksum, char **path_p) {
  const char  *checksumpart = checksum;
  char        *dir_path = NULL;
  int         status;
  int         failed = 0;

  asprintf(&dir_path, "%s/data", db_path);
  /* Two cases: either there are files, or a .nofiles file and directories */
  do {
    char *temp_path = NULL;

    /* If we can find a .nofiles file, then go down one more directory */
    asprintf(&temp_path, "%s/.nofiles", dir_path);
    status = testfile(temp_path, 0);
    free(temp_path);

    if (! status) {
      char *new_dir_path = NULL;

      asprintf(&new_dir_path, "%s/%c%c", dir_path, checksumpart[0],
        checksumpart[1]);
      checksumpart += 2;
      free(dir_path);
      dir_path = new_dir_path;
    }
  } while (! status);
  /* Return path */
  asprintf(path_p, "%s/%s", dir_path, (char *) checksumpart);
  if (testdir(dir_path, 1)) {
    failed = 1;
  }
  free(dir_path);
  return failed;
}

static int db_load(const char *filename, list_t list) {
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
      char      *string = malloc(size);
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
            asprintf(&db_data.host, string);
            break;
          case 2:   /* Path */
            asprintf(&db_data.filedata.path, string);
            break;
          case 3:   /* Type */
            if (sscanf(string, "%c", &letter) != 1) {
              failed = field;
            }
            db_data.filedata.metadata.type = type_mode(letter);
            break;
          case 4:   /* Size */
            if (sscanf(string, "%ld", &db_data.filedata.metadata.size) != 1) {
              failed = field;
            }
            break;
          case 5:   /* Modification time */
            if (sscanf(string, "%ld", &db_data.filedata.metadata.mtime) != 1) {
              failed = field;
            }
            break;
          case 6:   /* User */
            if (sscanf(string, "%u", &db_data.filedata.metadata.uid) != 1) {
              failed = field;
            }
            break;
          case 7:   /* Group */
            if (sscanf(string, "%u", &db_data.filedata.metadata.gid) != 1) {
              failed = field;
            }
            break;
          case 8:   /* Permissions */
            if (sscanf(string, "%o", &db_data.filedata.metadata.mode) != 1) {
              failed = field;
            }
            break;
          case 9:   /* Link */
            asprintf(&db_data.link, string);
            break;
          case 10:  /* Checksum */
            /* TODO: this is just transitory */
            if (strcmp(string, "N")) {
              strcpy(db_data.filedata.checksum, string);
            } else {
              db_data.filedata.checksum[0] = '\0';
            }
            break;
          case 11:  /* Date in */
            if (sscanf(string, "%ld", &db_data.date_in) != 1) {
              failed = field;
            }
            break;
          case 12:  /* Date out */
            if (sscanf(string, "%ld", &db_data.date_out) != 1) {
              failed = field;
            }
            break;
          case 13:  /* Mark */
            if (strcmp(string, "-")) {
              failed = field;
            }
            break;
          default:
            failed = field;
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
        fprintf(stderr, "db: failed to read list file: wrong format (%d)\n",
          field);
      } else {
        db_data_p = malloc(sizeof(db_data_t));
        *db_data_p = db_data;
        list_add(db_list, db_data_p);
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

static int db_save(const char *filename, list_t list) {
  char *temp_path;
  FILE *writefile;
  int  failed = 0;

  asprintf(&temp_path, "%s/%s.part", db_path, filename);
  if ((writefile = fopen(temp_path, "w")) != NULL) {
    char *dest_path = NULL;
    list_entry_t entry = NULL;

    while ((entry = list_next(list, entry)) != NULL) {
      db_data_t *db_data = list_entry_payload(entry);
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

static void db_data_get(const void *payload, char **string_p) {
  const db_data_t *db_data = payload;

  if (db_data->date_out == 0) {
    /* '@' > '9' */
    asprintf(string_p, "%s %s %c", db_data->host, db_data->filedata.path, '@');
  } else {
    /* ' ' > '0' */
    asprintf(string_p, "%s %s %11ld", db_data->host, db_data->filedata.path,
      db_data->date_out);
  }
}

static int db_organize(char *path, int number) {
  DIR           *directory;
  struct dirent *dir_entry;
  char          *nofiles = malloc(strlen(path) + strlen("/.nofiles") + 1);
  int           failed   = 0;

  /* Already organized? */
  sprintf(nofiles, "%s/.nofiles", path);
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
      /* Add 2: '\0' and '/' */
      source_path = malloc(strlen(path) + strlen(dir_entry->d_name) + 2);
      sprintf(source_path, "%s/%s", path, dir_entry->d_name);
      if (metadata_get(source_path, &metadata)) {
        fprintf(stderr, "db: organize: cannot get metadata: %s\n", source_path);
        failed = 2;
      } else if (S_ISDIR(metadata.type) && (dir_entry->d_name[2] != '-')) {
        /* Add 2: '\0' and '/' */
        char *dest_path = malloc(strlen(source_path) + 2);

        /* Create two-letter directory */
        strcpy(dest_path, path);
        strcat(dest_path, "/");
        strncat(dest_path, dir_entry->d_name, 2);
        if (testdir(dest_path, 1) == 2) {
          failed = 2;
        } else {
          /* Create destination path */
          strcat(dest_path, "/");
          strcat(dest_path, &dir_entry->d_name[2]);
          /* Move directory accross, changing its name */
          if (rename(source_path, dest_path)) {
            failed = 1;
          }
        }
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

static int db_write(const char *mount_path, const char *path, size_t size, char *checksum) {
  char  *source_path = NULL;
  char  *temp_path   = NULL;
  char  *dest_path   = NULL;
  char  temp_checksum[36];
  FILE  *writefile;
  FILE  *readfile;
  int   index = 0;
  int   delete = 0;
  int   failed = 0;

  /* File to read from */
  asprintf(&source_path, "%s/%s", mount_path, path);

  /* Temporary file to write to */
  asprintf(&temp_path, "%s/filedata", db_path);

  /* Copy file locally */
  if (copy(source_path, temp_path, temp_checksum) != size) {
    if (! terminating()) {
      /* Don't signal errors on termination */
      fprintf(stderr, "db: write: failed to copy file: %s\n", path);
    }
    failed = 1;
  }
  free(source_path);

  /* Get file final location */
  if (! failed && getdir(temp_checksum, &dest_path) == 2) {
    fprintf(stderr, "db: write: failed to get dir for: %s\n", temp_checksum);
    failed = 1;
  }

  /* Make sure our checksum is unique */
  if (! failed) {
    do {
      char  *final_path = NULL;
      int   differ = 0;

      /* Complete checksum with index */
      sprintf((char *) checksum, "%s-%u", temp_checksum, index);
      asprintf(&final_path, "%s-%u/", dest_path, index);
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
                char    buffer1[10240];
                char    buffer2[10240];
                differ = 0;

                do {
                  size_t length = fread(buffer1, 1, 10240, readfile);

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
                  delete = 1;
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
  if (failed || delete) {
    remove(temp_path);
  }
  free(temp_path);

  /* Make sure we won't exceed the file number limit */
  if (! failed) {
    /* dest_path is /path/to/checksum/data */
    char *delim = strrchr(dest_path, '/');

    if (delim != NULL) {
      *delim = '\0';
      /* Now dest_path is /path/to/checksum */
      if ((delim = strrchr(dest_path, '/')) != NULL) {
        *delim = '\0';
        /* Now dest_path is /path/to */
        db_organize(dest_path, 256);
      }
    }
  }
  free(dest_path);
  return failed;
}

int db_open(const char *path) {
  char *data_path = NULL;
  char *lock_path = NULL;
  FILE *file;
  int  status = 0;

  asprintf(&db_path, path);

  /* Check that data dir exists, create it (no need to lock) */
  asprintf(&data_path, "%s/data", db_path);
  status = testdir(data_path, 1);
  free(data_path);
  if (status == 2) {
    return 2;
  }
  status = 0;

  /* Take lock */
  asprintf(&lock_path, "%s/lock", db_path);
  if ((file = fopen(lock_path, "r")) != NULL) {
    pid_t pid;

    /* Lock already taken */
    fscanf(file, "%d", &pid);
    fclose(file);
    if (pid != 0) {
      /* Find out whether process is still running, if not, reset lock */
      kill(pid, 0);
      if (errno == ESRCH) {
        fprintf(stderr, "db: open: lock reset\n");
        remove(lock_path);
      } else {
        fprintf(stderr, "db: open: lock taken by process with pid %d\n", pid);
        status = 2;
      }
    } else {
      fprintf(stderr, "db: open: lock taken by an unidentified process!\n");
      status = 2;
    }
  }
  if (! status) {
    if ((file = fopen(lock_path, "w")) != NULL) {
      /* Lock taken */
      fprintf(file, "%u\n", getpid());
      fclose(file);
    } else {
      /* Lock cannot be taken */
      fprintf(stderr, "db: open: cannot take lock\n");
      status = 2;
    }
  }

  /* Read database list */
  if (! status) {
    db_list = list_new(db_data_get);
    switch (db_load("list", db_list)) {
      case 0:
        if (verbosity() > 1) {
          printf(" -> Loaded database list: %u file(s)\n", list_size(db_list));
        }
        break;
      case 1:
        if (verbosity() > 1) {
          printf(" -> Initialized database list\n");
        }
        break;
      case 2:
        remove(lock_path);
        status = 2;
    }
  }
  free(lock_path);
  return status;
}

static void db_list_free(list_t list) {
  list_entry_t entry = NULL;

  while ((entry = list_next(list, entry)) != NULL) {
    db_data_t *db_data = list_entry_payload(entry);

    free(db_data->host);
    free(db_data->link);
    free(db_data->filedata.path);
  }
  list_free(list);
}

void db_close(void) {
  char      *temp_path  = NULL;
  time_t    gmt;
  struct tm gmt_brokendown;

  /* Save list for month day */
  if (verbosity() > 1) {
    printf(" -> Saving database list: %u file(s)\n", list_size(db_list));
  }
  if ((time(&gmt) != -1) && (gmtime_r(&gmt, &gmt_brokendown) != NULL)) {
    char *daily_list = NULL;

    asprintf(&daily_list, "list_%2u", gmt_brokendown.tm_mday);
    db_save(daily_list, db_list);
    free(daily_list);
  }

  /* Save list as main */
  db_save("list", db_list);
  db_list_free(db_list);

  /* Release lock */
  asprintf(&temp_path, "%s/lock", db_path);
  remove(temp_path);
  free(temp_path);
  free(db_path);
}

/* Need to compare only for matching paths */
static int parse_compare(void *db_data_p, void *filedata_p) {
  const db_data_t *db_data  = db_data_p;
  filedata_t      *filedata = filedata_p;
  int             result;

  /* No more data in database list: add to added list */
  if (db_data_p == NULL) {
    return 1;
  }

  /* Ignore removed files */
  if (db_data->date_out != 0) {
    return -2;
  }

  /* Ignore when paths do not match */
  if ((db_data->filedata.path[backup_path_length] != '/')
   || strncmp(db_data->filedata.path, backup_path, backup_path_length)) {
    return -2;
  }

  /* No more data in file list: add to missing list */
  if (filedata_p == NULL) {
    return -1;
  }

  /* If paths differ, that's all we want to check */
  if ((result = strcmp(&db_data->filedata.path[backup_path_length + 1],
      filedata->path))) {
    return result;
  }
  /* If the file has been modified, just return 1 or -1 and that should do */
  result = memcmp(&db_data->filedata.metadata, &filedata->metadata,
    sizeof(metadata_t));

  /* If it's a file and size and mtime are the same, copy checksum accross */
  if (S_ISREG(db_data->filedata.metadata.type)) {
    if (! strcmp(db_data->filedata.checksum, "N")) {
      /* Checksum missing, add to missing list */
      return -1;
    } else
    if ((db_data->filedata.metadata.size == filedata->metadata.size)
     || (db_data->filedata.metadata.mtime == filedata->metadata.mtime)) {
      strcpy(filedata->checksum, db_data->filedata.checksum);
    }
  }
  return result;
}

int db_parse(const char *host, const char *real_path,
    const char *mount_path, list_t file_list) {
  list_t        added_files_list;
  list_t        removed_files_list;
  list_entry_t  entry  = NULL;
  int           failed = 0;

  /* Compare list with db list for matching host */
  asprintf(&backup_path, real_path);
  backup_path_length = strlen(real_path);
  list_compare(db_list, file_list, &added_files_list, &removed_files_list,
    parse_compare);
  free(backup_path);
  backup_path = NULL;

  /* Deal with new/modified data first */
  if (added_files_list != NULL) {
    static int   copied = 0;
    static off_t volume = 0;

    if (verbosity() > 2) {
      printf(" --> Files to add: %u\n", list_size(added_files_list));
    }
    while ((entry = list_next(added_files_list, entry)) != NULL) {
      if (! terminating()) {
        filedata_t *filedata = list_entry_payload(entry);
        db_data_t  *db_data  = malloc(sizeof(db_data_t));

        asprintf(&db_data->host, host);
        db_data->filedata.metadata = filedata->metadata;
        asprintf(&db_data->filedata.path, "%s/%s", real_path,
          filedata->path);
        db_data->link = NULL;
        db_data->date_in = time(NULL);
        db_data->date_out = 0;
        /* Save new data */
        if (S_ISREG(filedata->metadata.type)) {
          if (filedata->checksum[0] != '\0') {
            /* We need the old checksum here! */
            strcpy(db_data->filedata.checksum, filedata->checksum);
          } else if (db_write(mount_path, filedata->path,
              db_data->filedata.metadata.size, db_data->filedata.checksum)) {
            /* Write failed, need to go on */
            failed = 1;
            if (! terminating()) {
              /* Don't signal errors on termination */
              fprintf(stderr, "db: parse: %s: %s, ignoring\n",
                  strerror(errno), db_data->filedata.path);
            }
            strcpy(db_data->filedata.checksum, "N");
          }
        } else {
          strcpy(db_data->filedata.checksum, "");
        }
        if (S_ISLNK(filedata->metadata.type)) {
          char *full_path = NULL;
          char *string = malloc(FILENAME_MAX);
          int size;

          asprintf(&full_path, "%s/%s", mount_path, filedata->path);
          if ((size = readlink(full_path, string, FILENAME_MAX)) < 0) {
            failed = 1;
            fprintf(stderr, "db: parse: %s: %s, ignoring\n",
              strerror(errno), db_data->filedata.path);
          } else {
            db_data->link = malloc(size + 1);
            strncpy(db_data->link, string, size);
            db_data->link[size] = '\0';
          }
          free(full_path);
          free(string);
        }
        list_add(db_list, db_data);
        if ((++copied >= 1000)
         || ((volume += db_data->filedata.metadata.size) >= 10000000)) {
          copied = 0;
          volume = 0;
          if (verbosity() > 2) {
            printf(" --> Files left to add: %u\n",
              list_size(added_files_list));
            printf(" --> Saving database list: %u file(s)\n",
              list_size(db_list));
          }
          db_save("list", db_list);
        }
      }
      /* This only unlists the data */
      list_remove(added_files_list, entry);
    }
    /* The list should be empty now */
    if (list_size(added_files_list) != 0) {
      fprintf(stderr, "db: parse: added_files_list not empty, ignoring\n");
    } else {
      list_free(added_files_list);
    }
  }

  /* Deal with removed/modified data */
  if (removed_files_list != NULL) {
    if (verbosity() > 2) {
      printf(" --> Files to remove: %u\n", list_size(removed_files_list));
    }
    while ((entry = list_next(removed_files_list, entry)) != NULL) {
      db_data_t *db_data = list_entry_payload(entry);

      /* Same data as in db_list */
      db_data->date_out = time(NULL);
      /* This only unlists the data */
      list_remove(removed_files_list, entry);
    }
    if (list_size(removed_files_list) != 0) {
      fprintf(stderr, "db: parse: removed_files_list not empty, ignoring\n");
    } else {
      list_free(removed_files_list);
    }
  }

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

  if (getdir(checksum, &temp_path)) {
    fprintf(stderr, "db: read: failed to get dir for: %s\n", checksum);
    return 2;
  }
  asprintf(&source_path, "%s/data", temp_path);
  free(temp_path);
  temp_path = NULL;

  /* Open temporary file to write to */
  asprintf(&temp_path, "%s.part", path);

  /* Copy file to temporary name (size not checked: checksum suffices) */
  if (copy(source_path, temp_path, temp_checksum) < 0) {
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

int db_scan(const char *checksum) {
  int failed = 0;

  if (checksum == NULL) {
    list_entry_t entry = NULL;
    int files = list_size(db_list);

    if (verbosity() > 1) {
      printf(" -> Scanning database list: %u file(s)\n", list_size(db_list));
    }
    while ((entry = list_next(db_list, entry)) != NULL) {
      db_data_t *db_data = list_entry_payload(entry);

      if ((verbosity() > 2) && ((files & 0x3FF) == 0)) {
        printf(" --> Files left to go: %u\n", files);
      }
      files--;
      if (db_scan(db_data->filedata.checksum)) {
        failed = 1;
        fprintf(stderr, "File data missing for checksum %s\n",
          db_data->filedata.checksum);
      }
      if (terminating()) {
        return 3;
      }
    }
  } else if ((checksum[0] != '\0') && strcmp(checksum, "N")) {
    char *path = NULL;

    if (getdir(checksum, &path)) {
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
  return failed;
}

int db_check(const char *checksum) {
  int failed = 0;

  if (checksum == NULL) {
    list_entry_t entry = NULL;
    int          files = list_size(db_list);

    if (verbosity() > 1) {
      printf(" -> Checking database list: %u file(s)\n", list_size(db_list));
    }
    while ((entry = list_next(db_list, entry)) != NULL) {
      db_data_t *db_data = list_entry_payload(entry);

      if ((verbosity() > 2) && ((files & 0xFF) == 0)) {
        printf(" --> Files left to go: %u\n", files);
      }
      files--;
      switch (db_check(db_data->filedata.checksum)) {
        case 1:
          failed = 1;
          fprintf(stderr, "File data missing for checksum %s\n",
            db_data->filedata.checksum);
          break;
        case 2:
          failed = 1;
          fprintf(stderr, "File data corrupted for checksum %s\n",
            db_data->filedata.checksum);
          break;
      }
      if (terminating()) {
        return 3;
      }
    }
    return failed;
  } else if (strcmp(checksum, "N")) {
    char       *path = NULL;

    if (getdir(checksum, &path)) {
      failed = 1;
    } else {
      FILE *readfile;
      char *check_path = NULL;

      asprintf(&check_path, "%s/data", path);
      /* Read file to compute checksum, compare with expected */
      if ((readfile = fopen(check_path, "r")) == NULL) {
        failed = 1;
      } else {
        char       check[36];
        EVP_MD_CTX ctx;
        size_t     rlength;

        EVP_DigestInit(&ctx, EVP_md5());
        while (! feof(readfile) && ! terminating()) {
          char buffer[10240];

          rlength = fread(buffer, 1, 10240, readfile);
          EVP_DigestUpdate(&ctx, buffer, rlength);
        }
        fclose(readfile);
        /* Re-use length */
        EVP_DigestFinal(&ctx, (unsigned char *) check, &rlength);
        md5sum(check, rlength);
        if (strcmp(check, checksum)) {
          failed = 2;
        }
      }
      free(check_path);
    }
    free(path);
  }
  return failed;
}
