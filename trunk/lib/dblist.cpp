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

      DbData db_data(buffer, bsize);
      if (db_data.in() == 0) {
        failed = 1;
        cerr << "dblist: load: " << filename << ": Corrupted, line " << line
          << endl;
        errno = EUCLEAN;
      } else {
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

int DbList::save_new(
    const string& path,
    const string& filename,
    bool          backup) {
  FILE    *writefile;
  int     failed = 0;

  string dest_path = path + "/" + filename;
  string temp_path = dest_path + ".part";
  if ((writefile = fopen(temp_path.c_str(), "w")) != NULL) {
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
        last_out = 0;
        asprintf(&last_prefix, i->prefix().c_str());
      }
      if ((last_path == NULL) || strcmp(last_path, i->data()->path().c_str())){
        fprintf(writefile, "\t%s\n", i->data()->path().c_str());
        free(last_path);
        last_path = NULL;
        last_out = 0;
        asprintf(&last_path, i->data()->path().c_str());
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
        fprintf(writefile, "\t\t%ld\t\n", i->out());
      }
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
