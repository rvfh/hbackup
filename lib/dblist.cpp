/*
     Copyright (C) 2007  Herve Fache

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

#include <iostream>
#include <string>
#include <errno.h>

using namespace std;

#include "dbdata.h"
#include "dblist.h"
#include "hbackup.h"

using namespace hbackup;

int DbList::load(
    const string& path,
    const string& filename,
    unsigned int  offset) {
  string source_path = path + "/" + filename;
  int    failed      = 0;

  FILE* readfile = fopen(source_path.c_str(), "r");
  if (readfile == NULL) {
    // errno set by fopen
    failed = -1;
  } else {
    /* Read the file into memory */
    char    *buffer = NULL;
    size_t  bsize   = 0;

    // errno set by load_v*
    if (getline(&buffer, &bsize, readfile) >= 0) {
      if (buffer[0] != '#') {
        rewind(readfile);
        failed = load_v0(readfile);
      } else {
        // Only v1 for now
        failed = load_v1(readfile);
      }
    }
    free(buffer);
    fclose(readfile);
  }

  if (failed != 0) {
    cerr << "dblist: load: " << filename << ": " << strerror(errno) << endl;
  }
  return failed;
}

int DbList::load_v0(FILE* readfile, unsigned int offset) {
  /* Read the file into memory */
  char          *buffer = NULL;
  size_t        bsize   = 0;
  unsigned int  line    = 0;
  int           failed  = 0;

  errno = 0;
  while ((getline(&buffer, &bsize, readfile) >= 0) && ! failed) {
    if (++line <= offset) {
      continue;
    }

    DbData db_data(buffer, bsize);
    if (db_data.in() == 0) {
      failed = -1;
      cerr << "dblist: load: file corrupted, line " << line << endl;
      errno = EUCLEAN;
    } else {
      add(db_data);
    }
  }
  free(buffer);

  // Unclutter
  if (size() > 0) {
    SortedList<DbData>::iterator prev = begin();
    SortedList<DbData>::iterator i = prev;
    i++;
    while (i != end()) {
      if ((*i->data() == *prev->data())
        && (i->checksum() == prev->checksum())) {
        prev->setOut(i->out());
        i = erase(i);
      } else {
        i++;
        prev++;
      }
    }
  }

  return failed;
}

int DbList::load_v1(FILE* readfile, unsigned int offset) {
  /* Read the file into memory */
  char          *buffer = NULL;
  size_t        bsize   = 0;
  unsigned int  line    = 0;
  ssize_t       size    = 0;
  int           failed  = 0;

  char*         prefix  = NULL;
  char*         path    = NULL;

  SortedList<DbData>::iterator db_data;
  bool          end_found = false;

  errno = 0;
  while (((size = getline(&buffer, &bsize, readfile)) >= 0) && ! failed) {
    if (++line <= offset) {
      continue;
    }
    // Remove ending '\n'
    buffer[strlen(buffer) - 1] = '\0';
    if (buffer[0] == '#') {
      end_found = true;
      break;
    } else
    if (buffer[0] != '\t') {
      free(prefix);
      prefix = NULL;
      asprintf(&prefix, buffer);
    } else
    if (buffer[1] != '\t') {
      free(path);
      path = NULL;
      asprintf(&path, &buffer[1]);
      db_data = end();
    } else {
      char* start  = &buffer[2];
      char* value  = (char *) malloc(size + 1);
      int   failed = 0;
      int   fields = 7;
      // Fields
      time_t    db_time;        // DB time
      char      type;           // file type ('?' if metadata not available)
      time_t    mtime;          // time of last modification
      long long size;           // on-disk size, in bytes
      uid_t     uid;            // user ID of owner
      gid_t     gid;            // group ID of owner
      mode_t    mode;           // permissions
      string    checksum;       // file checksum
      string    link;           // what the link points to

      for (int field = 1; field <= fields; field++) {
        // Get tabulation position
        char* delim = strchr(start, '\t');
        if (delim == NULL) {
          failed = 1;
        } else {
          // Get string portion
          strncpy(value, start, delim - start);
          value[delim - start] = '\0';
          /* Extract data */
          switch (field) {
            case 1:   /* DB timestamp */
              if (sscanf(value, "%ld", &db_time) != 1) {
                failed = -1;
              }
              break;
            case 2:   /* Type */
              if (sscanf(value, "%c", &type) != 1) {
                failed = 2;
              } else if (type == '-') {
                fields = 2;
              } else if ((type == 'f') || (type == 'l')) {
                fields++;
              }
              if (db_data != end()) {
                db_data->setOut(db_time);
                db_data = end();
              }
              break;
            case 3:   /* Size */
              if (sscanf(value, "%lld", &size) != 1) {
                failed = 2;
              }
              break;
            case 4:   /* Modification time */
              if (sscanf(value, "%ld", &mtime) != 1) {
                failed = 2;
              }
              break;
            case 5:   /* User */
              if (sscanf(value, "%u", &uid) != 1) {
                failed = 2;
              }
              break;
            case 6:   /* Group */
              if (sscanf(value, "%u", &gid) != 1) {
                failed = 2;
              }
              break;
            case 7:   /* Permissions */
              if (sscanf(value, "%o", &mode) != 1) {
                failed = 2;
              }
              break;
            case 8:  /* Checksum or Link */
                if (type == 'f') {
                  checksum = value;;
                } else if (type == 'l') {
                  link = value;;
                }
              break;
          }
          start = delim + 1;
        }
        if (failed) {
          cerr << "dblist: load: file corrupted, line " << line << endl;
          errno = EUCLEAN;
          break;
        }
      }
      free(value);
      if ((type != '-') && (failed == 0)) {
        DbData data(File(path, link, type, mtime, size, uid, gid, mode),
          db_time);
        data.setPrefix(prefix);
        data.setChecksum(checksum);
        db_data = add(data);
      }
    }
  }
  free(buffer);

  if (! end_found) {
    cerr << "dblist: load: file end not found" << endl;
    errno = EUCLEAN;
    failed = -1;
  }

 return failed;
}

int DbList::save(
    const string& path,
    const string& filename,
    bool          backup) {
  FILE    *writefile;
  int     failed = 0;

  string dest_path = path + "/" + filename;
  string temp_path = dest_path + ".part";
  if ((writefile = fopen(temp_path.c_str(), "w")) != NULL) {
    fprintf(writefile, "# version 1\n");
    char*  last_prefix = NULL;
    char*  last_path   = NULL;
    time_t last_out    = 0;
    for (SortedList<DbData>::iterator i = begin(); i != end(); i++) {
      if ((last_prefix == NULL) || strcmp(last_prefix, i->prefix().c_str())) {
        fprintf(writefile, "%s\n", i->prefix().c_str());
        free(last_prefix);
        last_prefix = NULL;
        free(last_path);
        last_path = NULL;
        asprintf(&last_prefix, i->prefix().c_str());
      }
      if ((last_path == NULL) || strcmp(last_path, i->data()->path().c_str())){
        if (last_out != 0) {
          fprintf(writefile, "\t\t%ld\t-\t\n", last_out);
          last_out = 0;
        }
        fprintf(writefile, "\t%s\n", i->data()->path().c_str());
        free(last_path);
        last_path = NULL;
        asprintf(&last_path, i->data()->path().c_str());
      } else {
        if (last_out != i->in()) {
          fprintf(writefile, "\t\t%ld\t-\t\n", last_out);
          last_out = 0;
        }
      }
      fprintf(writefile, "\t\t%ld\t%c\t%lld\t%ld\t%u\t%u\t%o\t",
        i->in(), i->data()->type(), i->data()->size(), i->data()->mtime(),
        i->data()->uid(), i->data()->gid(), i->data()->mode());
      if (i->data()->type() == 'f') {
        fprintf(writefile, "%s\t", i->checksum().c_str());
      }
      if (i->data()->type() == 'l') {
        fprintf(writefile, "%s\t", i->data()->link().c_str());
      }
      fprintf(writefile, "\n");
      if (i->out() != 0) {
        last_out = i->out();
      }
    }
    // Last one...
    if (last_out != 0) {
      fprintf(writefile, "\t\t%ld\t-\t\n", last_out);
    }
    fprintf(writefile, "#\n");
    fclose(writefile);

    /* All done */
    if (backup && rename(dest_path.c_str(), (dest_path + "~").c_str())) {
      if (verbosity() > 3) {
        cerr << "dblist: save: cannot create backup" << endl;
      }
      failed = 2;
    }
    if (rename(temp_path.c_str(), dest_path.c_str())) {
      if (verbosity() > 3) {
        cerr << "dblist: save: cannot rename file" << endl;
      }
      failed = 2;
    }
  } else {
    if (verbosity() > 3) {
      cerr << "dblist: save: cannot create file" << endl;
    }
    failed = 2;
  }
  return failed;
}

int DbList::save_journal(
    const string& path,
    const string& filename,
    unsigned int  offset) {
  FILE          *writefile;
  int           failed = 0;
  unsigned int  line = 0;

  string dest_path = path + "/" + filename;
  if ((writefile = fopen(dest_path.c_str(), "a")) != NULL) {
    SortedList<DbData>::iterator i;
    for (i = begin(); i != end(); i++) {
      if (++line > offset) {
        fprintf(writefile, "%s\n", i->line().c_str());
      }
    }
    fclose(writefile);
  } else {
    failed = 2;
  }
  return failed;
}

int DbList::open(
    const string& path,
    const string& filename) {
  if (load(path, filename)) {
    // This should not fail
    cerr << "dblist: failed to load " << filename << " items list" << endl;
    return 2;
  }
  _open = true;

  if (verbosity() > 2) {
    cout << " --> Database " << filename << " items part open (contents: "
      << size() << " file";
    if (size() != 1) {
      cout << "s";
    }
    cout << ")" << endl;
  }

  if (size() == 0) {
    return 1;
  }
  return 0;
}

int DbList::close(
    const string& path,
    const string& filename) {
  if (! _open) {
    cerr << "dblist: cannot close " << filename << " items list, not open"
      << endl;
    return 2;
  }
  if (save(path, filename, true)) {
    cerr << "dblist: failed to save " << filename << " items list" << endl;
    return 2;
  }
  int list_size = size();
  clear();
  _open = false;

  if (verbosity() > 2) {
    cout << " --> Database " << filename << " items part closed (contents: "
      << list_size << " file";
    if (list_size != 1) {
      cout << "s";
    }
    cout << ")" << endl;
  }
  return 0;
}
