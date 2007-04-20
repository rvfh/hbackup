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
#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <dirent.h>
#include <errno.h>

using namespace std;

#include "list.h"
#include "files.h"
#include "dbdata.h"
#include "dblist.h"
#include "hbackup.h"

using namespace hbackup;

int DbList::load(
    const string& path,
    const string& filename,
    unsigned int  offset) {
  string  source_path;
  FILE    *readfile;
  int     failed = 0;

  errno = 0;
  source_path = path + "/" + filename;
  if ((readfile = fopen(source_path.c_str(), "r")) != NULL) {
    /* Read the file into memory */
    char          *buffer = NULL;
    size_t        bsize   = 0;
    unsigned int  line    = 0;

    while ((getline(&buffer, &bsize, readfile) >= 0) && ! failed) {
      if (++line <= offset) {
        continue;
      }

      char      *start = buffer;
      char      *delim;
      char      *value = new char[bsize];
      // Data
      string    access_path;
      string    path;
      string    link;
      string    checksum;
      char      letter;
      time_t    mtime;
      long long fsize;
      uid_t     uid;
      gid_t     gid;
      mode_t    mode;
      // DB data
      time_t    in;
      time_t    out;
      int       field = 0;

      while (((delim = strchr(start, '\t')) != NULL) && ! failed) {
        /* Get string portion */
        strncpy(value, start, delim - start);
        value[delim - start] = '\0';
        /* Extract data */
        failed = 0;
        switch (++field) {
          case 1:   /* Prefix */
            access_path = value;
            break;
          case 2:   /* Path */
            path = value;
            break;
          case 3:   /* Type */
            if (sscanf(value, "%c", &letter) != 1) {
              failed = 2;
            }
            break;
          case 4:   /* Size */
            if (sscanf(value, "%lld", &fsize) != 1) {
              failed = 2;
            }
            break;
          case 5:   /* Modification time */
            if (sscanf(value, "%ld", &mtime) != 1) {
              failed = 2;
            }
            break;
          case 6:   /* User */
            if (sscanf(value, "%u", &uid) != 1) {
              failed = 2;
            }
            break;
          case 7:   /* Group */
            if (sscanf(value, "%u", &gid) != 1) {
              failed = 2;
            }
            break;
          case 8:   /* Permissions */
            if (sscanf(value, "%o", &mode) != 1) {
              failed = 2;
            }
            break;
          case 9:   /* Link */
            link = value;
            break;
          case 10:  /* Checksum */
            // TODO remove
            if (value != "N") {
              checksum = value;
            } else {
              checksum = "";
            }
            break;
          case 11:  /* Date in */
            if (sscanf(value, "%ld", &in) != 1) {
              failed = 2;
            }
            break;
          case 12:  /* Date out */
            if (sscanf(value, "%ld", &out) != 1) {
              failed = 2;
            }
            break;
          default:
            failed = 2;
        }
        start = delim + 1;
      }
      free(value);
      if (field != 12) {
        failed = 1;
      }
      if (failed) {
        cerr << "dblist: load: " << filename << ": Corrupted, line " << line
          << endl;
        errno = EUCLEAN;
      } else {
        File    data(access_path, path, link, File::typeMode(letter), mtime,
          fsize, uid, gid, mode);
        DbData  db_data(in, out, checksum, data);
        add(db_data);
      }
    }
    free(buffer);
    fclose(readfile);

    // Unclutter
    if (size() > 0) {
      SortedList<DbData>::iterator prev = begin();
      SortedList<DbData>::iterator i = prev;
      i++;
      while (i != end()) {
        if ((i->data() == prev->data())
         && (i->checksum() == prev->checksum())) {
          prev->setOut(i->out());
          i = erase(i);
        } else {
          i++;
          prev++;
        }
      }
    }
  } else {
    // errno set by fopen
    failed = 1;
    cerr << "dblist: load: " << filename << ": " << strerror(errno) << endl;
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
    SortedList<DbData>::iterator i;
    for (i = begin(); i != end(); i++) {
      fprintf(writefile, "%s\n", i->line().c_str());
    }
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
