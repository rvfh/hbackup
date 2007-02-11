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

using namespace std;

#include <iostream>
#include <string>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>

#include "list.h"
#include "tools.h"
#include "metadata.h"
#include "common.h"
#include "filters.h"
#include "filelist.h"
#include "db.h"

#define CHUNK 10240

typedef struct {
  char        *host;
  filedata_t  filedata;
  char        *link;
  time_t      date_in;
  time_t      date_out;
} db_data_t;

static List   *db_list = NULL;
static string _path = "";

/* Size of path being backed up */
static int    backup_path_length = 0;

static int db_load(const string &filename, List *list) {
  string  source_path;
  FILE    *readfile;
  int     failed = 0;

  errno = 0;
  source_path = _path + "/" + filename;
  if ((readfile = fopen(source_path.c_str(), "r")) != NULL) {
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
        errno = EUCLEAN;
      } else {
        db_data_p = new db_data_t;
        *db_data_p = db_data;
        list->add(db_data_p);
      }
    }
    free(buffer);
    fclose(readfile);
  } else {
    // errno set by fopen
    failed = 1;
  }
  return failed;
}

static int db_save(const string& filename, List *list) {
  string  temp_path;
  FILE    *writefile;
  int     failed = 0;

  temp_path = _path + "/" + filename + ".part";
  if ((writefile = fopen(temp_path.c_str(), "w")) != NULL) {
    string        dest_path;
    list_entry_t  *entry = NULL;

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
    dest_path = _path + "/" + filename;
    failed = rename(temp_path.c_str(), dest_path.c_str());
  } else {
    failed = 2;
  }
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

static int db_organize(const string& path, int number) {
  DIR           *directory;
  struct dirent *dir_entry;
  string        nofiles;
  int           failed   = 0;

  /* Already organized? */
  nofiles = path + "/.nofiles";
  if (! testfile(nofiles, 0)) {
    return 0;
  }
  /* Find out how many entries */
  if ((directory = opendir(path.c_str())) == NULL) {
    return 1;
  }
  while (((dir_entry = readdir(directory)) != NULL) && (number > 0)) {
    number--;
  }
  /* Decide what to do */
  if (number == 0) {
    rewinddir(directory);
    while ((dir_entry = readdir(directory)) != NULL) {
      metadata_t  metadata;
      string      source_path;

      /* Ignore . and .. */
      if (! strcmp(dir_entry->d_name, ".")
       || ! strcmp(dir_entry->d_name, "..")) {
        continue;
      }
      source_path = path + "/" + dir_entry->d_name;
      if (metadata_get(source_path.c_str(), &metadata)) {
        cerr << "db: organize: cannot get metadata: " << source_path << endl;
        failed = 2;
      } else if (S_ISDIR(metadata.type) && (dir_entry->d_name[2] != '\0')
        /* If we crashed, we might have some two-letter dirs already */
              && (dir_entry->d_name[2] != '-')) {
        /* If we've reached the point where the dir is ??-?, stop! */
        string  dir_path;

        /* Create two-letter directory */
        dir_path = string(path) + "/" + dir_entry->d_name[0] + dir_entry->d_name[1];
        if (testdir(dir_path, 1) == 2) {
          failed = 2;
        } else {
          string  dest_path;

          /* Create destination path */
          dest_path = dir_path + "/" + &dir_entry->d_name[2];
          /* Move directory accross, changing its name */
          if (rename(source_path.c_str(), dest_path.c_str())) {
            failed = 1;
          }
        }
      }
    }
    if (! failed) {
      testfile(nofiles, 1);
    }
  }
  closedir(directory);
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

static int db_write(
    string          mount_path,
    const char      *path,
    const db_data_t *db_data,
    char            *checksum,
    int             compress) {
  string  source_path;
  string  temp_path;
  string  dest_path;
  char    checksum_source[36];
  char    checksum_dest[36];
  int     index = 0;
  int     deleteit = 0;
  int     failed = 0;
  off_t   size_source;
  off_t   size_dest;

  /* File to read from */
  source_path = mount_path + "/" + path;

  /* Temporary file to write to */
  temp_path = _path + "/filedata";

  /* Copy file locally */
  if (zcopy(source_path, temp_path, &size_source, &size_dest,
      checksum_source, checksum_dest, compress)) {
    failed = 1;
  } else

  /* Check size_source size */
  if (size_source != db_data->filedata.metadata.size) {
    cerr << "db: write: file copy incomplete: " << path << endl;
    failed = 1;
  } else

  /* Get file final location */
  if (getdir(_path, checksum_source, dest_path) == 2) {
    cerr << "db: write: failed to get dir for: " << checksum_source << endl;
    failed = 1;
  } else {
    /* Make sure our checksum is unique */
    do {
      string  final_path = dest_path + "-";
      bool    differ = false;

      /* Complete checksum with index */
      sprintf((char *) checksum, "%s-%u", checksum_source, index);
      stringstream ss;
      string str;
      ss << index;
      ss >> str;
      final_path += str;
      if (! testdir(final_path, 1)) {
        /* Directory exists */
        string try_path;

        try_path = final_path + "/data";
        if (! testfile(try_path, 0)) {
          /* A file already exists, let's compare */
          metadata_t try_md;
          metadata_t temp_md;

          differ = 1;

          /* Compare sizes only */
          if (! metadata_get(try_path.c_str(), &try_md)
          && ! metadata_get(temp_path.c_str(), &temp_md)
          && (try_md.size == temp_md.size)) {
            differ = 0;
          }
        }
      }
      if (! differ) {
        dest_path = final_path;
        break;
      }
      index++;
    } while (true);

    /* Now move the file in its place */
    dest_path += "/data";
    if (rename(temp_path.c_str(), dest_path.c_str())) {
      cerr << "db: write: failed to move file " << temp_path
        << " to " << dest_path << ": " << strerror(errno);
      failed = 1;
    }
  }

  /* If anything failed, delete temporary file */
  if (failed || deleteit) {
    remove(temp_path.c_str());
  }

  /* Save redundant information together with data */
  if (! failed) {
    /* dest_path is /path/to/checksum/data */
    unsigned int pos = dest_path.rfind('/');

    if (pos != string::npos) {
      string        listfile;
      List          *list;
      list_entry_t  *entry;
      db_data_t     new_db_data = *db_data;

      dest_path.erase(pos);
      /* Now dest_path is /path/to/checksum */
      listfile = dest_path.substr(_path.size() + 1) + "/list";
      list = new List(db_data_get);
      if (db_load(listfile, list) == 2) {
        cerr << "db: write: failed to open data list" << endl;
      } else {
        strcpy(new_db_data.filedata.checksum, checksum_dest);
        entry = list->add(&new_db_data);
        db_save(listfile, list);
        list->remove(entry);
      }
      db_list_free(list);
    }
  }

  /* Make sure we won't exceed the file number limit */
  if (! failed) {
    /* dest_path is /path/to/checksum */
    unsigned int pos = dest_path.rfind('/');

    if (pos != string::npos) {
      dest_path.erase(pos);
      /* Now dest_path is /path/to */
      db_organize(dest_path, 256);
    }
  }

  return failed;
}

static int db_obsolete(
    const string& prefix,
    const string& path,
    const string& checksum) {
  string  listfile;
  string  temp_path;
  int     failed = 0;
  List    *list;

  if (getdir(_path, checksum, temp_path)) {
    cerr << "db: obsolete: failed to get dir for: " << checksum << endl;
    return 2;
  }
  listfile = temp_path.substr(_path.size() + 1) + "/list";

  list = new List(db_data_get);
  if (db_load(listfile.c_str(), list) == 2) {
    cerr << "db: obsolete: failed to open data list" << endl;
  } else {
    db_data_t     *db_data;
    list_entry_t  *entry;
    string        string;

    string = prefix + " " + path + " @";
    list->find(string.c_str(), NULL, &entry);
    if (entry != NULL) {
      db_data = (db_data_t *) (list_entry_payload(entry));
      db_data->date_out = time(NULL);
      db_save(listfile.c_str(), list);
    }
  }
  db_list_free(list);

  return failed;
}

static int db_lock(const string& path) {
  string  lock_path;
  FILE    *file;
  int     status = 0;

  /* Set the database path that we just locked as default */
  _path = path;
  lock_path = _path + "/lock";

  /* Try to open lock file for reading: check who's holding the lock */
  if ((file = fopen(lock_path.c_str(), "r")) != NULL) {
    pid_t pid;

    /* Lock already taken */
    fscanf(file, "%d", &pid);
    fclose(file);
    if (pid != 0) {
      /* Find out whether process is still running, if not, reset lock */
      kill(pid, 0);
      if (errno == ESRCH) {
        cerr << "db: lock: lock reset" << endl;
        remove(lock_path.c_str());
      } else {
        cerr << "db: lock: lock taken by process with pid " << pid << endl;
        status = 2;
      }
    } else {
      cerr << "db: lock: lock taken by an unidentified process!" << endl;
      status = 2;
    }
  }

  /* Try to open lock file for writing: lock */
  if (! status) {
    if ((file = fopen(lock_path.c_str(), "w")) != NULL) {
      /* Lock taken */
      fprintf(file, "%u\n", getpid());
      fclose(file);
    } else {
      /* Lock cannot be taken */
      cerr << "db: lock: cannot take lock" << endl;
      status = 2;
    }
  }
  return status;
}

static void db_unlock(const string& path) {
  string lock_path = _path + "/lock";

  _path = "";
  remove(lock_path.c_str());
}


int db_open(const string& path) {
  int status;

  /* Take lock */
  if (db_lock(path)) {
    errno = ENOLCK;
    return 2;
  }

  /* Check that data dir and list file exist, if not create them */
  _path = path;
  string data_path = _path + "/data";
  status = testdir(data_path, 1);
  if (status == 1) {
    data_path = _path + "/list";
    if (testfile(data_path, 1) == 2) {
      status = 2;
    } else if (verbosity() > 0) {
      cout << "Database initialized" << endl;
    }
  }
  /* status = 0 => data was there */
  /* status = 1 => initialized db */
  /* status = 2 => error */

  /* Read database active items list */
  if (status != 2) {
    db_list = new List(db_data_get);
    db_load("list", db_list);
    switch (errno) {
      case 0:
        if ((verbosity() > 0) && (status == 0)) {
          printf("Database opened (active contents: %u file(s))\n",
            db_list->size());
        }
        break;
      case ENOENT:
        if (! status) {
          /* TODO There was a data directory, but no list. Attempt recovery */
          cerr << "db: open: list missing" << endl;
          status = 2;
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
    cerr << "db: close: failed to save active items list" << endl;
  }
  active_list->deselect();
  delete active_list;

  /* Save removed list */
  if (db_save("removed", removed_list)) {
    cerr << "db: close: failed to save removed items list" << endl;
  }
  removed_list->deselect();
  delete removed_list;

  /* Release lock */
  db_unlock(_path);
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

int db_parse(
    const string& host,
    const string& real_path,
    const string& mount_path,
    List          *file_list) {
  List          *selected_files_list = new List(db_data_get);
  List          *added_files_list = new List();
  List          *removed_files_list = new List();
  list_entry_t  *entry      = NULL;
  char          *pathslash  = NULL;
  int           failed      = 0;

  /* Compare list with db list for matching host */
  backup_path_length = real_path.size() + 1;
  asprintf(&pathslash, "%s/", real_path.c_str());
  db_list->select(pathslash, parse_select, selected_files_list, NULL);
  free(pathslash);
  selected_files_list->compare(file_list, added_files_list,
    removed_files_list, parse_compare);
  selected_files_list->deselect();

  /* Deal with new/modified data first */
  if (added_files_list->size() != 0) {
    /* Static to be global to all shares */
    static int    copied        = 0;
    static off_t  volume        = 0;
    double        sizetobackup  = 0.0;
    double        sizebackedup  = 0.0;
    int           filestobackup = 0;
    int           filesbackedup = 0;

    /* Determine volume to be copied */
    if (verbosity() > 2) {
      while ((entry = added_files_list->next(entry)) != NULL) {
        filedata_t *filedata = (filedata_t *) (list_entry_payload(entry));

        filestobackup++;
        if (S_ISREG(filedata->metadata.type)) {
          sizetobackup += double(filedata->metadata.size);
        }
      }
      printf(" --> Files to add: %u (%0.f bytes)\n",
        added_files_list->size(), sizetobackup);
    }

    while (((entry = added_files_list->next(entry)) != NULL) && ! terminating()) {
      filedata_t *filedata = (filedata_t *) (list_entry_payload(entry));
      db_data_t  *db_data  = new db_data_t;

      asprintf(&db_data->host, "%s", host.c_str());
      db_data->filedata.metadata = filedata->metadata;
      asprintf(&db_data->filedata.path, "%s/%s", real_path.c_str(), filedata->path);
      db_data->link = NULL;
      db_data->date_in = time(NULL);
      db_data->date_out = 0;
      /* Save new data */
      if (S_ISREG(filedata->metadata.type)) {
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
              fprintf(stderr, "\r%s: %s, ignoring\n",
                  strerror(errno), db_data->filedata.path);
            }
            strcpy(db_data->filedata.checksum, "N");
          }
        }
        if (verbosity() > 2) {
          sizebackedup += double(filedata->metadata.size);
        }
      } else {
        strcpy(db_data->filedata.checksum, "");
      }
      if (S_ISLNK(filedata->metadata.type)) {
        string  full_path = mount_path + "/" + filedata->path;
        char    *link     = new char[FILENAME_MAX];
        int     size;

        if ((size = readlink(full_path.c_str(), link, FILENAME_MAX)) < 0) {
          failed = 1;
          fprintf(stderr, "\r%s: %s, ignoring\n",
            strerror(errno), db_data->filedata.path);
        } else {
          link[size] = '\0';
          asprintf(&db_data->link, "%s", link);
        }
        free(link);
      }
      db_list->add(db_data);
      if ((++copied >= 1000)
      || ((volume += db_data->filedata.metadata.size) >= 10000000)) {
        copied = 0;
        volume = 0;
        db_save("list", db_list);
      }
      if (verbosity() > 2) {
        filesbackedup++;
        if (sizetobackup != 0) {
          printf(" --> Done: %d/%d (%.1f%%)\r", filesbackedup, filestobackup, (sizebackedup / sizetobackup) * 100.0);
        } else {
          printf(" --> Done: %d/%d\r", filesbackedup, filestobackup);
        }
      }
    }
  } else if (verbosity() > 2) {
    cout << " --> No files to add" << endl;
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

int db_read(const string& path, const string& checksum) {
  string  source_path;
  string  temp_path;
  char    temp_checksum[256];
  int     failed = 0;

  if (getdir(_path, checksum, source_path)) {
    cerr << "db: read: failed to get dir for: " << checksum << endl;
    return 2;
  }
  source_path += "/data";

  /* Open temporary file to write to */
  temp_path = path + ".part";

  /* Copy file to temporary name (size not checked: checksum suffices) */
  if (zcopy(source_path.c_str(), temp_path.c_str(), NULL, NULL, temp_checksum, NULL, 0)) {
    cerr << "db: read: failed to copy file: " << source_path << endl;
    failed = 2;
  } else

  /* Verify that checksums match before overwriting current final destination */
  if (checksum.substr(0, strlen(temp_checksum)) != string(temp_checksum)) {
    cerr << "db: read: checksums don't match: " << source_path
      << " " << temp_checksum << endl;
    failed = 2;
  } else

  /* All done */
  if (rename(temp_path.c_str(), path.c_str())) {
    cerr << "db: read: failed to rename file to " << strerror(errno)
      << ": " << path << endl;
    failed = 2;
  }

  if (failed) {
    remove(temp_path.c_str());
  }

  return failed;
}

int db_scan(const string& local_db_path, const string& checksum) {
  int failed = 0;

  if (local_db_path != "") {
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
    if (checksum == "") {
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
        if (strlen(db_data->filedata.checksum)
         && db_scan("", db_data->filedata.checksum)) {
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
      string  path;

      if (getdir(_path, checksum, path)) {
        failed = 1;
      } else {
        string  test_path = path + "/data";

        if (testfile(test_path, 0)) {
          failed = 2;
        }
      }
    }
  }

  if (local_db_path != "") {
    db_list_free(db_list);
    db_unlock(local_db_path);
  }

  return failed;
}

int db_check(const string& local_db_path, const string& checksum) {
  int failed = 0;
  char checksum_real[256];

  if (local_db_path != "") {
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
    if (checksum == "") {
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
        if (strlen(db_data->filedata.checksum)) {
          failed = db_check("", db_data->filedata.checksum);
        }
        if (terminating()) {
          failed = 3;
          break;
        }
      }
    } else if ((checksum[0] != 'N') && (checksum[0] != '\0')) {
      string  path;

      if (getdir(_path, checksum, path)) {
        failed = 2;
      } else {
        string  check_path;
        string  listfile;
        List    *list;

        listfile = path.substr(_path.size() + 1) + "/list";
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
            check_path = path + "/data";
            if (getchecksum(check_path.c_str(), checksum_real) == 2) {
              cerr << "File data missing for checksum " << checksum << endl;
              failed = 1;
            } else
            if (strncmp(db_data->filedata.checksum, checksum_real,
                  strlen(checksum_real)) != 0) {
              if (! terminating()) {
                cerr << "File data corrupted for checksum " << checksum
                  << " (found to be " << checksum_real << ")" << endl;
              }
              failed = 1;
            }

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
      }
    }
  }

  if (local_db_path != "") {
    db_list_free(db_list);
    db_unlock(local_db_path);
  }

  return failed;
}
