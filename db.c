/* Herve Fache

20061008 Creation
*/

/* List file contents:
 *  prefix        (given in the format: 'protocol://host/share')
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
#include <time.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <openssl/md5.h>
#include <openssl/evp.h>
#include "metadata.h"
#include "list.h"
#include "db.h"

typedef struct {
  char        prefix[FILENAME_MAX];
  metadata_t  metadata;
  char        link[FILENAME_MAX];
  char        checksum[256];
  time_t      date_in;
  time_t      date_out;
} db_data_t;

static list_t *db_list = NULL;
static char db_path[FILENAME_MAX];

static int testdir(const char *path, int create) {
  DIR  *directory;

  if ((directory = opendir(path)) == NULL) {
    if (create && mkdir(path, 0777)) {
      fprintf(stderr, "db: failed to create directory: %s\n", path);
      return 2;
    }
    return 1;
  }
  closedir(directory);
  return 0;
}

static int testfile(const char *path) {
  FILE  *file;

  if ((file = fopen(path, "r")) == NULL) {
    return 1;
  }
  fclose(file);
  return 0;
}

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

static int copy(const char *source_path, const char *dest_path, char *checksum) {
  FILE        *writefile;
  FILE        *readfile;
  char        buffer[10240];
  EVP_MD_CTX  ctx;
  size_t      length;


  /* Open file to read from */
  if ((readfile = fopen(source_path, "r")) == NULL) {
    fprintf(stderr, "db: failed to open source file: %s\n", source_path);
    return 2;
  }

  /* Open file to write to */
  if ((writefile = fopen(dest_path, "w")) == NULL) {
    fclose(readfile);
    fprintf(stderr, "db: failed to open destination file: %s\n", dest_path);
    return 2;
  }

  /* We shall copy and compute the checksum in one go */
  EVP_DigestInit(&ctx, EVP_md5());
  while (! feof(readfile)) {
    length = fread(buffer, 1, 10240, readfile);
    EVP_DigestUpdate(&ctx, buffer, length);
    do {
      length -= fwrite(buffer, 1, length, writefile);
    } while (length != 0);
  }
  fclose(readfile);
  fclose(writefile);
  EVP_DigestFinal(&ctx, (unsigned char *) checksum, &length);
  md5sum(checksum, length);
  return 0;
}

static int getdir(const char *checksum, char *path) {
  const char  *checksumpart = checksum;
  char        dir_path[FILENAME_MAX];
  int         status;

  strcpy(dir_path, db_path);
  strcat(dir_path, "data/");
  /* Two cases: either there are files, or a .nofiles file and directories */
  do {
    char temp_path[FILENAME_MAX];

    /* If we can find a .nofiles file, then go down one more directory */
    strcpy(temp_path, dir_path);
    strcat(temp_path, ".nofiles");
    if (! (status = testfile(temp_path))) {
      strncat(dir_path, (char *) checksumpart, 2);
      strcat(dir_path, "/");
      checksumpart += 2;
    }
  } while (! status);
  /* Return path */
  strcpy(path, dir_path);
  strcat(path, (char *) checksumpart);
  if (testdir(dir_path, 1)) {
    return 1;
  }
  return 0;
}

/* Read file in path, [compress and] store it, return checksum */
/*static int db_organize(void) {
  return 1;
}
*/
static int db_load(const char *filename, list_t list) {
  char source_path[FILENAME_MAX];
  FILE *readfile;

  strcpy(source_path, db_path);
  strcat(source_path, filename);
  if ((readfile = fopen(source_path, "r")) != NULL) {
    /* Read the file into memory */
    char *buffer = malloc(FILENAME_MAX);
    size_t size;

    while (getline(&buffer, &size, readfile) >= 0) {
      db_data_t db_data;
      char      *start = buffer;
      char      *delim;
/* This does not work...
{
char string[FILENAME_MAX];
char string2[FILENAME_MAX];

size = sscanf(buffer, "'%s' '%s'", string, string2);
printf("Size read: %u - %s\n", size, string);
}
*/
      if (buffer[0] != '\'') {
        fprintf(stderr, "db: failed to read list file: wrong format (1)\n");
        continue;
      }
      start = &buffer[1];
      delim = strchr(start, '\'');
      if (delim == NULL) {
        fprintf(stderr, "db: failed to read list file: wrong format (2)\n");
        continue;
      }
      strncpy(db_data.prefix, start, delim - start);
printf("Read: %s\n", db_data.prefix);
      start = delim;
      start++;
      delim = strchr(start, '\'');
      start = delim;
      start++;
/*      strncpy(db_data.metadata.path, start, delim - start);
printf("Read: %s\n", db_data.metadata.path);*/
    }
    fclose(readfile);
    return 0;
  }
fprintf(stderr, "db: failed to read list file: not implemented\n");
  return 1;
}

static int db_save(const char *filename, list_t list) {
  char temp_path[FILENAME_MAX];
  char dest_path[FILENAME_MAX];
  FILE *writefile;

  strcpy(dest_path, db_path);
  strcat(dest_path, filename);
  strcpy(temp_path, dest_path);
  strcat(temp_path, ".part");
  if ((writefile = fopen(temp_path, "w")) != NULL) {
    list_entry_t entry = NULL;

    while ((entry = list_next(list, entry)) != NULL) {
      db_data_t *db_data = list_entry_payload(entry);

      /* The link could also be stored as data... */
      fprintf(writefile, "'%s' '%s' %c %ld %ld %u %u 0%o '%s' %s %ld %ld %c\n",
        db_data->prefix, db_data->metadata.path,
        type_letter(db_data->metadata.type), db_data->metadata.size,
        db_data->metadata.mtime, db_data->metadata.uid, db_data->metadata.gid,
        db_data->metadata.mode, db_data->link, db_data->checksum,
        db_data->date_in, db_data->date_out, '-');
    }
    fclose(writefile);

    /* All done */
    if (! rename(temp_path, dest_path)) {
      return 0;
    }
  }
  fprintf(stderr, "db: failed to write list file: %s\n", filename);
  return 2;
}

static void db_data_get(const void *payload, char *string) {
  const db_data_t *db_data = payload;
  time_t          date_out = db_data->date_out;

  if (date_out == 0) {
    /* '@' > '9' */
    sprintf(string, "%s/%s %c", db_data->prefix, db_data->metadata.path, '@');
  } else {
    /* ' ' > '0' */
    sprintf(string, "%s/%s %11ld", db_data->prefix, db_data->metadata.path, date_out);
  }
}

static int db_write(const char *path, char *checksum) {
  char  temp_path[FILENAME_MAX];
  char  dest_path[FILENAME_MAX];
  char  temp_checksum[256];
  FILE  *writefile;
  FILE  *readfile;
  int   index = 0;
  int   delete = 0;
  int   failed = 0;

  /* Open temporary file to write to */
  strcpy(temp_path, db_path);
  strcat(temp_path, "filedata");

  /* Get file final location */
  if (copy(path, temp_path, temp_checksum) || (getdir(temp_checksum, dest_path) == 2)) {
    fprintf(stderr, "failed to copy or get dir for: %s\n", path);
    failed = 1;
  }

  /* Make sure our checksum is unique */
  if (!failed) do {
    char  final_path[FILENAME_MAX];
    int   differ = 0;

    /* Complete checksum with index */
    sprintf((char *) checksum, "%s-%u", temp_checksum, index);
    sprintf(final_path, "%s-%u/", dest_path, index);
    if (! testdir(final_path, 1)) {
      /* Directory exists */
      char  try_path[FILENAME_MAX];

      strcpy(try_path, final_path);
      strcat(try_path, "data");
      if (! testfile(try_path)) {
        /* A file already exists, let's compare */
        /* TODO checking sizes first sounds like a good idea */
        differ = 1;

        /* Compare try_path to temp_path */
        if ((readfile = fopen(try_path, "r")) != NULL) {
          if ((writefile = fopen(temp_path, "r")) != NULL) {
            char    buffer1[10240];
            char    buffer2[10240];
            differ = 0;

            do {
              size_t length = fread(buffer1, 1, 10240, readfile);
              /* Does fread read as much as possible? */
              if ((fread(buffer2, 1, length, writefile) != length) || strncmp(buffer1, buffer2, length)) {
                differ = 1;
                break;
              }
            } while (! feof(readfile) && ! feof(writefile));
            /* Is there more data in one of the files? */
            if (!differ) {
              differ = fread(buffer1, 1, 1, readfile) || fread(buffer2, 1, 1, writefile);
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
    if (! differ) {
      strcpy(dest_path, final_path);
      break;
    }
    index++;
  } while (1);

  /* Now move the file in its place */
  strcat(dest_path, "data");
  if (!failed && rename(temp_path, dest_path)) {
    fprintf(stderr, "failed to move file %s to %s: %s\n", strerror(errno), temp_path, dest_path);
    failed = 1;
  }

  /* If anything failed, delete temporary file */
  if (failed || delete) {
    remove(temp_path);
  }
  if (failed) {
    fprintf(stderr, "db: failed to backup contents for: %s\n", path);
    return 2;
  }
  return 0;
}

int db_open(const char *path) {
  char temp_path[FILENAME_MAX];
  FILE *file;
  int status = 0;

  strcpy(db_path, path);
  strcat(db_path, "/");

  /* Check that data exists (no need to lock) */
  strcpy(temp_path, db_path);
  strcat(temp_path, "data");
  if (testdir(temp_path, 1) == 2) {
    fprintf(stderr, "db: open: cannot create directory: %s\n", temp_path);
    return 2;
  }

  /* Take lock */
  strcpy(temp_path, db_path);
  strcat(temp_path, "lock");
  if ((file = fopen(temp_path, "r")) != NULL) {
    pid_t pid;

    /* Lock already taken */
    fscanf(file, "%d", &pid);
    fclose(file);
    fprintf(stderr, "db: open: lock taken by process with pid %d\n", pid);
    if (pid != 0) {
      /* TODO Find out whether process is still running, if not, reset lock */
    }
    return 2;
  }
  if ((file = fopen(temp_path, "w")) != NULL) {
    /* Lock taken */
    fprintf(file, "%u\n", getpid());
    fclose(file);
  } else {
    /* Lock cannot be taken */
    fprintf(stderr, "db: open: cannot lock\n");
    return 2;
  }

  /* Read database list */
  db_list = list_new(db_data_get);
  status = db_load("list", db_list);
  if (status == 2) {
    remove(temp_path);
    return 2;
  }
  return status;
}

void db_close(void) {
  char temp_path[FILENAME_MAX];

  /* Save list */
  db_save("list", db_list);
  list_free(db_list);

  /* Release lock */
  strcpy(temp_path, db_path);
  strcat(temp_path, "lock");
  remove(temp_path);
}

static int parse_compare(const void *db_data_p, const void *metadata_p) {
  const db_data_t   *db_data  = db_data_p;
  const metadata_t  *metadata = metadata_p;
  int               result;

  /* Removed files should be ignored */
  if (db_data->date_out != 0) {
    return 2;
  }
  /* If paths differ, that's all we want to check */
  if ((result = strcmp(db_data->metadata.path, metadata->path))) {
    return result;
  }
  /* If the file has been modified, just return 1 or -1 and that should do */
  return memcmp(&db_data->metadata, metadata, sizeof(metadata_t));
}

int db_parse(const char *prefix, const char *mount_point, list_t file_list) {
  list_t        added_files_list;
  list_t        removed_files_list;
  list_entry_t  entry  = NULL;
  int           failed = 0;

  /* Compare list with db list for matching prefix */
  list_compare(db_list, file_list, &added_files_list, &removed_files_list,
    parse_compare);

  /* Deal with new/modified data first */
  if (added_files_list != NULL) {
    int mount_point_length = strlen(mount_point);

    if (mount_point[mount_point_length] != '/') {
      mount_point_length++;
    }
    while ((entry = list_next(added_files_list, entry)) != NULL) {
      metadata_t *metadata = list_entry_payload(entry);
      db_data_t  *db_data  = malloc(sizeof(db_data_t));

      strcpy(db_data->prefix, prefix);
      db_data->metadata = *metadata;
      strcpy(db_data->metadata.path, &metadata->path[mount_point_length]);
      strcpy(db_data->link, "");
      db_data->date_in = time(NULL);
      db_data->date_out = 0;
      /* Save new data */
      if (S_ISREG(metadata->type)) {
        if (db_write(metadata->path, db_data->checksum)) {
          /* Write failed, need to go on */
          failed = 1;
          fprintf(stderr, "db: parse: %s: %s, ignoring\n",
              strerror(errno), metadata->path);
          strcpy(db_data->checksum, "N");
        }
      } else {
        strcpy(db_data->checksum, "N");
      }
      if (S_ISLNK(metadata->type)) {
        int size = readlink(metadata->path, db_data->link, FILENAME_MAX);

        if (size < 0) {
          failed = 1;
          fprintf(stderr, "db: parse: %s: %s, ignoring\n",
              strerror(errno), metadata->path);
          strcpy(db_data->link, "");
        }
      } else {
        strcpy(db_data->link, "");
      }
      list_add(db_list, db_data);
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
  char  source_path[FILENAME_MAX];
  char  temp_path[FILENAME_MAX];
  char  temp_checksum[256];

  if (getdir(checksum, source_path)) {
    fprintf(stderr, "db: read: failed to get dir for: %s\n", checksum);
    return 2;
  }
  strcat(source_path, "/data");

  /* Open temporary file to write to */
  strcpy(temp_path, path);
  strcat(temp_path, ".part");

  /* Copy file to temporary name */
  if (copy(source_path, temp_path, temp_checksum)) {
    fprintf(stderr, "db: read: failed to copy file: %s\n", source_path);
    remove(temp_path);
    return 2;
  }

  /* Verify that checksums match before overwriting current final destination */
  if (strncmp(checksum, temp_checksum, strlen(temp_checksum))) {
    fprintf(stderr, "db: read: checksums don't match: %s %s\n", source_path, temp_checksum);
    remove(temp_path);
    return 2;
  }

  /* All done */
  if (rename(temp_path, path)) {
    fprintf(stderr, "db: read: failed to rename file to %s: %s\n", strerror(errno), path);
    return 2;
  }

  return 0;
}

int db_scan(const char *checksum) {
  return 1;
}

int db_check(const char *checksum) {
  return 1;
}
