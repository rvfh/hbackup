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
 *  prefix        (given in the format: 'protocol://host')
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
#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <dirent.h>
#include <errno.h>

#include "list.h"
#include "files.h"
#include "db.h"
#include "hbackup.h"

bool DbData::operator<(const DbData& right) const {
  if (_data < right._data) return true;
  if (right._data < _data) return false;
  // Equal then...
  return _in < right._in;
}

bool DbData::operator!=(const DbData& right) const {
  return (_in != right._in) || (_out != right._out)
   || (_checksum != right._checksum) || (_data != right._data);
}

string DbData::line(bool nodates) const {
  string  output = _data.line(nodates) + "\t" + _checksum;
  char*   numbers = NULL;
  time_t  in, out;

  if (nodates) {
    in = _in != 0;
    out = _out != 0;
  } else {
    in = _in;
    out = _out;
  }

  asprintf(&numbers, "%ld\t%ld", in, out);
  output += "\t" + string(numbers) + "\t";
  delete numbers;
  return output;
}

int Database::load(const string &filename, SortedList<DbData>& list) {
  string  source_path;
  FILE    *readfile;
  int     failed = 0;

  errno = 0;
  source_path = _path + "/" + filename;
  if ((readfile = fopen(source_path.c_str(), "r")) != NULL) {
    /* Read the file into memory */
    char *buffer = NULL;
    size_t size  = 0;

    while ((getline(&buffer, &size, readfile) >= 0) && ! failed) {
      char      *start = buffer;
      char      *delim;
      char      *value = new char[size];
      int       line = 1;
      // Data
      string    access_path;
      string    path;
      string    link;
      string    checksum;
      char      letter;
      time_t    mtime;
      off_t     size;
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
            if (sscanf(value, "%ld", &size) != 1) {
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
            checksum = value;
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
        line++;
      }
      free(value);
      if (field != 12) {
        failed = 1;
      }
      if (failed) {
        cerr << "db: open: list corrupted line " << line << endl;
        errno = EUCLEAN;
      } else if (path[1] != '$') {
        File    data(access_path, path, link, File::typeMode(letter), mtime,
          size, uid, gid, mode);
        DbData  db_data(in, out, checksum, data);
        list.add(db_data);
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

int Database::save(const string& filename, SortedList<DbData>& list) {
  FILE    *writefile;
  int     failed = 0;

  string dest_path = _path + "/" + filename;
  string temp_path = dest_path + ".part";
  if ((writefile = fopen(temp_path.c_str(), "w")) != NULL) {
    SortedList<DbData>::iterator i;
    for (i = list.begin(); i != list.end(); i++) {
      fprintf(writefile, "%s\n", i->line().c_str());
    }
    fclose(writefile);

    /* All done */
    failed = rename(temp_path.c_str(), dest_path.c_str());
  } else {
    failed = 2;
  }
  return failed;
}

int Database::organize(const string& path, int number) {
  DIR           *directory;
  struct dirent *dir_entry;
  string        nofiles;
  int           failed   = 0;

  /* Already organized? */
  nofiles = path + "/.nofiles";
  if (! File::testReg(nofiles, 0)) {
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
      /* Ignore . and .. */
      if (! strcmp(dir_entry->d_name, ".")
       || ! strcmp(dir_entry->d_name, "..")) {
        continue;
      }
      File file_data(path, dir_entry->d_name);
      string source_path = path + "/" + dir_entry->d_name;
      if (file_data.type() == 0) {
        cerr << "db: organize: cannot get metadata: " << source_path << endl;
        failed = 2;
      } else
      if (S_ISDIR(file_data.type())
       // If we crashed, we might have some two-letter dirs already
       && (file_data.name().size() != 2)
       // If we've reached the point where the dir is ??-?, stop!
       && (file_data.name()[2] != '-')) {
        // Create two-letter directory
        string dir_path = path + "/" + file_data.name().substr(0,2);
        if (File::testDir(dir_path, 1) == 2) {
          failed = 2;
        } else {
          // Create destination path
          string dest_path = dir_path + "/" + file_data.name().substr(2);
          // Move directory accross, changing its name
          if (rename(source_path.c_str(), dest_path.c_str())) {
            failed = 1;
          }
        }
      }
    }
    if (! failed) {
      File::testReg(nofiles, 1);
    }
  }
  closedir(directory);
  return failed;
}

int Database::write(
    const string&   path,
    DbData&         db_data,
    int             compress) {
  string  temp_path;
  string  dest_path;
  string  checksum_source;
  string  checksum_dest;
  string  checksum;
  int     index = 0;
  int     deleteit = 0;
  int     failed = 0;
  off_t   size_source;
  off_t   size_dest;

  /* Temporary file to write to */
  temp_path = _path + "/filedata";
  /* Copy file locally */
  if (File::zcopy(path, temp_path, &size_source, &size_dest, &checksum_source,
   &checksum_dest, compress)) {
    failed = 1;
  } else

  /* Check size_source size */
  if (size_source != db_data.data().size()) {
    cerr << "db: write: file copy incomplete: " << path << endl;
    failed = 1;
  } else

  /* Get file final location */
  if (getDir(checksum_source, dest_path, true) == 2) {
    cerr << "db: write: failed to get dir for: " << checksum_source << endl;
    failed = 1;
  } else {
    /* Make sure our checksum is unique */
    do {
      string  final_path = dest_path + "-";
      bool    differ = false;

      /* Complete checksum with index */
      stringstream ss;
      string str;
      ss << index;
      ss >> str;
      checksum = checksum_source + "-" + str;
      final_path += str;
      if (! File::testDir(final_path, 1)) {
        /* Directory exists */
        string try_path;

        try_path = final_path + "/data";
        if (File::testReg(try_path, 0) == 0) {
          /* A file already exists, let's compare */
          File try_md(try_path);
          File temp_md(temp_path);

          differ = (try_md.size() != temp_md.size());
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

  // Report checksum
  if (! failed) {
    db_data.setChecksum(checksum);
  } else {
    db_data.setChecksum("N");
  }

  /* Save redundant information together with data */
  if (! failed) {
    /* dest_path is /path/to/checksum/data */
    unsigned int pos = dest_path.rfind('/');

    if (pos != string::npos) {
      string        listfile;

      dest_path.erase(pos);
      /* Now dest_path is /path/to/checksum */
      listfile = dest_path.substr(_path.size() + 1) + "/list";
      SortedList<DbData> list;
      if (load(listfile, list) == 2) {
        cerr << "db: write: failed to open data list" << endl;
        failed = 1;
      } else {
        DbData new_db_data(db_data);
        new_db_data.setChecksum(checksum_dest);
        list.add(new_db_data);
        save(listfile, list);
      }
    }
  }

  /* Make sure we won't exceed the file number limit */
  if (! failed) {
    /* dest_path is /path/to/checksum */
    unsigned int pos = dest_path.rfind('/');

    if (pos != string::npos) {
      dest_path.erase(pos);
      /* Now dest_path is /path/to */
      organize(dest_path, 256);
    }
  }

  return failed;
}

int Database::obsolete(const string& checksum, const File& file_data) {
  string  listfile;
  string  temp_path;
  int     failed = 0;

  if (getDir(checksum, temp_path, false)) {
    cerr << "db: obsolete: failed to get dir for: " << checksum << endl;
    return 2;
  }
  listfile = temp_path.substr(_path.size() + 1) + "/list";

  SortedList<DbData> list;
  if (load(listfile, list) == 2) {
    cerr << "db: obsolete: failed to open data list" << endl;
  } else {
    string search = file_data.prefix() + " " + file_data.path();
    SortedList<DbData>::iterator i = list.begin();
    while (i != list.end()) {
      if ((i->out() == 0)
       && (file_data.prefix() == i->data().prefix())
       && (file_data.path() == i->data().path())) {
        i->setOut();
        save(listfile, list);
        break;
      }
      i++;
    }
  }

  return failed;
}

int Database::lock() {
  string  lock_path;
  FILE    *file;
  int     status = 0;

  /* Set the database path that we just locked as default */
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

void Database::unlock() {
  string lock_path = _path + "/lock";
  remove(lock_path.c_str());
}

int Database::getDir(
    const string& checksum,
    string&       path,
    bool          create) {
  path = _path + "/data";
  int level = 0;

  // Two cases: either there are files, or a .nofiles file and directories
  do {
    // If we can find a .nofiles file, then go down one more directory
    string  temp_path = path + "/.nofiles";
    if (! File::testReg(temp_path, false)) {
      path += "/" + checksum.substr(level, 2);
      level += 2;
      if (File::testDir(path, create) == 2) {
        return 1;
      }
    } else {
      break;
    }
  } while (true);
  // Return path
  path += "/" + checksum.substr(level);
  return File::testDir(path, false);
}

int Database::open() {
  int status;

  /* Take lock */
  if (lock()) {
    errno = ENOLCK;
    return 2;
  }

  /* Check that mount dir exists, if not create it */
  if (File::testDir(_path + "/mount", true) == 2) {
    cerr << "db: open: cannot create mount point" << endl;
    status = 2;
  } else

  /* Check that data dir and list file exist, if not create them */
  if ((status = File::testDir(_path + "/data", true)) == 1) {
    if (File::testReg(_path + "/list", true) == 2) {
      cerr << "db: open: cannot create list file" << endl;
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
    load("list", _active);
    switch (errno) {
      case 0:
        if ((verbosity() > 0) && (status == 0)) {
          cout << "Database opened (active contents: "
            << _active.size() << " file";
          if (_active.size() != 1) {
            cout << "s";
          }
          cout << ")" << endl;
        }
        break;
      case ENOENT:
        if (! status) {
          /* TODO There was a data directory, but no list. Attempt recovery */
          cerr << "db: open: list missing" << endl;
          status = 2;
        }
        break;
      case EACCES:
        cerr << "db: open: permission to read list denied" << endl;
        status = 2;
        break;
      case EUCLEAN:
        status = 2;
        break;
      default:
        cerr << "db: open: " << strerror(errno) << endl;
        status = 2;
    }
  }

  if (status == 2) {
    unlock();
    return 2;
  }
  return 0;
}

int Database::close() {
  time_t          localtime;
  struct tm       localtime_brokendown;

  /* Save list for month day */
  if ((time(&localtime) != -1)
   && (localtime_r(&localtime, &localtime_brokendown) != NULL)) {
    char *daily_list = NULL;

    asprintf(&daily_list, "list_%02u", localtime_brokendown.tm_mday);
    save(daily_list, _active);
    free(daily_list);
  }

  // Save active list
  if (save("list", _active)) {
    cerr << "db: close: failed to save active items list" << endl;
  }
  int active_size = _active.size();
  _active.clear();

  /* Load previously removed items into removed list */
  /* TODO What do we do if this fails? Recover? */
  load("removed", _removed);
  _removed.unique();

  /* Save removed list */
  if (save("removed", _removed)) {
    cerr << "db: close: failed to save removed items list" << endl;
  }
  int removed_size = _removed.size();
  _removed.clear();

  /* Release lock */
  if (verbosity() > 0) {
    cout << "Database closed (active contents: " << active_size << " file";
    if (active_size != 1) {
      cout << "s";
    }
    cout << ", removed contents: " << removed_size << " file";
    if (removed_size != 1) {
      cout << "s";
    }
    cout << ")" << endl;
  }
  unlock();
  return 0;
}

int Database::parse(
    const string& prefix,
    const string& mounted_path,
    const string& mount_path,
    list<File>*   file_list) {
  int failed = 0;
  SortedList<DbData>                   added;
  // TODO can I make that a checksum-ordered list somehow?
  vector<SortedList<DbData>::iterator> removed;

  string full_path = prefix + "/" + mounted_path + "/";
  SortedList<DbData>::iterator entry_active = _active.begin();
  // Jump irrelevant first records
  while ((entry_active != _active.end())
   && (entry_active->data().fullPath(full_path.size()) < full_path)) {
    entry_active++;
  }
  SortedList<DbData>::iterator active_last = _active.end();

  list<File>::iterator          entry_file_list = file_list->begin();
  while (! terminating()) {
    bool file_add  = false;
    bool db_remove = false;
    bool db_erase  = false;
    bool same_file = false;

    // Check whether db data is (still) relevant
    if ((entry_active != _active.end()) && (active_last != entry_active)
     && (entry_active->data().fullPath(full_path.size()) > full_path)) {
      // Irrelevant rest of list
      entry_active = _active.end();
    }
    active_last = entry_active;

    // Check whether we have more work to do
    if ((entry_active == _active.end())
     && (entry_file_list == file_list->end())) {
      break;
    }

    // Deal with each case
    if (entry_active == _active.end()) {
      // End of db relevant data reached: add element
      file_add = true;
    } else
    if (entry_file_list == file_list->end()) {
      // End of file data reached: remove element
      db_remove = true;
    } else {
      int result = entry_active->data().path().substr(mounted_path.size() + 1).compare(entry_file_list->path());

      if (result < 0) {
        db_remove = true;
      } else
      if (result > 0) {
        file_add = true;
      } else
      if (entry_active->data().metadiffer(*entry_file_list)) {
        db_remove = true;
        file_add = true;

        /* If it's a file and size and mtime are the same, copy checksum accross */
        if (S_ISREG(entry_active->data().type())
         && (entry_active->data().size() == entry_file_list->size())
         && (entry_active->data().mtime() == entry_file_list->mtime())) {
          // Same file, just ownership changed or something
          same_file = true;
         }
      } else
      if (S_ISREG(entry_active->data().type())
       && (entry_active->checksum() == "N")) {
        // Previously failed write
        db_erase = true;
      } else {
        entry_active++;
        entry_file_list++;
      }
    }

    if (file_add) {
      // Create new record
      File file_data(*entry_file_list);
      file_data.setPrefix(prefix);
      file_data.setPath(mounted_path + "/" + entry_file_list->path());
      DbData db_data(file_data);
      if (same_file) {
        db_data.setChecksum(entry_active->checksum());
      }
      added.push_back(db_data);
      entry_file_list++;
    }

    // DB data is used above!
    if (db_remove) {
      // Push iterator
      removed.push_back(entry_active);
      entry_active++;
    } else
    if (db_erase) {
      entry_active = _active.erase(entry_active);
    }
  }

  /* Deal with new/modified data first */
  if (! added.empty()) {
    /* Static to be global to all shares */
    static int    copied        = 0;
    static off_t  volume        = 0;
    double        sizetobackup  = 0.0;
    double        sizebackedup  = 0.0;
    int           filestobackup = 0;
    int           filesbackedup = 0;
    SortedList<DbData>::iterator i;

    /* Determine volume to be copied */
    if (verbosity() > 2) {
      for (i = added.begin(); i != added.end(); i++) {
        /* Same data as in file_list */
        filestobackup++;
        if (S_ISREG(i->data().type()) && i->checksum().empty()) {
          sizetobackup += double(i->data().size());
        }
      }
      printf(" --> Files to add: %u (%0.f bytes)\n",
        added.size(), sizetobackup);
    }

    for (i = added.begin(); i != added.end(); i++) {
      if (terminating()) {
        break;
      }
      // Set checksum
      if (S_ISREG(i->data().type()) && i->checksum().empty()) {
        if (write(mount_path + i->data().path().substr(mounted_path.size()),
          *i) != 0) {
          /* Write failed, need to go on */
          failed = 1;
          if (! terminating()) {
            /* Don't signal errors on termination */
            cerr << "\r" << strerror(errno) << ": "
              << i->data().path() << ", ignoring" << endl;
          }
        }
        if (verbosity() > 2) {
          sizebackedup += double(i->data().size());
        }
      }
      // Save new data
      _active.add(*i);
      if ((++copied >= 1000)
      || ((volume += i->data().size()) >= 100000000)) {
        copied = 0;
        volume = 0;
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

  /* Deal with removed/modified data */
  if (! removed.empty()) {
    if (verbosity() > 2) {
      printf(" --> Files to remove: %u\n", removed.size());
    }
    for (unsigned int i = 0; i < removed.size(); i++) {
      if (terminating()) {
        break;
      }
      /* Update local list */
      if (S_ISREG(removed[i]->data().type())
       && ! removed[i]->checksum().empty()
       && (removed[i]->checksum() != "N")) {
        obsolete(removed[i]->checksum(), removed[i]->data());
        if (verbosity() > 3) {
          cout << " ---> " << removed[i]->checksum()
            << " (" << i + 1 << "/" << removed.size() << ")\r";
        }
      }

      // Mark removed
      removed[i]->setOut();
      // Add to removed list
      _removed.add(*removed[i]);
      // Remove from active list
      _active.erase(removed[i]);
    }
/*    if (verbosity() > 3) {
      cout << "                                       \r";
    }*/
  } else if (verbosity() > 2) {
    cout << " --> No files to remove\n";
  }

  /* Report errors */
  if (failed) {
    return 1;
  }
  return 0;
}

int Database::read(const string& path, const string& checksum) {
  string  source_path;
  string  temp_path;
  string  temp_checksum;
  int     failed = 0;

  if (getDir(checksum, source_path, false)) {
    cerr << "db: read: failed to get dir for: " << checksum << endl;
    return 2;
  }
  source_path += "/data";

  /* Open temporary file to write to */
  temp_path = path + ".part";

  /* Copy file to temporary name (size not checked: checksum suffices) */
  if (File::zcopy(source_path, temp_path, NULL, NULL, &temp_checksum, NULL, 0)) {
    cerr << "db: read: failed to copy file: " << source_path << endl;
    failed = 2;
  } else

  /* Verify that checksums match before overwriting current final destination */
  if (checksum.substr(0, temp_checksum.size()) != temp_checksum) {
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

int Database::scan(const string& checksum, bool thorough) {
  int     failed = 0;

  if (checksum == "") {
    int files = _active.size();

    if (verbosity() > 0) {
      cout << "Scanning database contents";
      if (thorough) {
        cout << " thoroughly";
      }
      cout << ": " << _active.size() << " file";
      if (_active.size() != 1) {
        cout << "s";
      }
      cout << endl;
    }

    SortedList<DbData>::iterator i;
    for (i = _active.begin(); i != _active.end(); i++) {
      if ((verbosity() > 2) && ((files & 0xFF) == 0)) {
        printf(" --> Files left to go: %u\n", files);
      }
      files--;
      if ((! i->checksum().empty()) && scan(i->checksum(), thorough)) {
        i->setChecksum("N");
        failed = 1;
        if (! terminating() && verbosity() > 1) {
          struct tm *time;
          cout << " -> Client:      " << i->data().prefix() << endl;
          cout << " -> File name:   " << i->data().path() << endl;
          time_t file_mtime = i->data().mtime();
          time = localtime(&file_mtime);
          printf(" -> Modified:    %04u-%02u-%02u %2u:%02u:%02u\n",
            time->tm_year + 1900, time->tm_mon + 1, time->tm_mday,
            time->tm_hour, time->tm_min, time->tm_sec);
          if (verbosity() > 2) {
            time_t local = i->in();
            time = localtime(&local);
            printf(" --> Seen first: %04u-%02u-%02u %2u:%02u:%02u\n",
              time->tm_year + 1900, time->tm_mon + 1, time->tm_mday,
              time->tm_hour, time->tm_min, time->tm_sec);
            if (i->out() != 0) {
              time_t local = i->out();
              time = localtime(&local);
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
  } else if (checksum[0] != 'N') {
    string  path;
    bool    filefailed = false;

    if (getDir(checksum, path, false)) {
      errno = ENODATA;
      filefailed = true;
      cerr << "db: scan: failed to get directory for checksum " << checksum << endl;
    } else
    if (File::testReg(path + "/data", 0)) {
      errno = ENOENT;
      filefailed = true;
      cerr << "db: scan: file data missing for checksum " << checksum << endl;
    } else
    if (thorough) {
      string  check_path = path + "/data";
      string  listfile = path.substr(_path.size() + 1) + "/list";
      SortedList<DbData> list;

      if (load(listfile, list) == 2) {
        errno = EUCLEAN;
        filefailed = true;
        cerr << "db: scan: failed to open list for checksum " << checksum << endl;
      } else {
        if (list.empty()) {
          errno = EUCLEAN;
          filefailed = true;
          cerr << "db: scan: list empty for checksum " << checksum << endl;
        } else {
          string checksum_real;

          /* Read file to compute checksum, compare with expected */
          if (File::getChecksum(check_path, checksum_real) == 2) {
            errno = ENOENT;
            filefailed = true;
            cerr << "db: scan: file data missing for checksum " << checksum << endl;
          } else
          if ((*list.begin()).checksum().substr(0, checksum_real.size())
           != checksum_real) {
            errno = EILSEQ;
            filefailed = true;
            if (! terminating()) {
              cerr << "db: scan: file data corrupted for checksum " << checksum
                << " (found to be " << checksum_real << ")" << endl;
            }
          }
        }
      }

      // Remove corrupted file if any
      if (filefailed) {
        remove(check_path.c_str());
      }
    }
    if (filefailed) {
      failed = 1;
    }
  }
  return failed;
}

int Database::recover() {
  return 1;
}
