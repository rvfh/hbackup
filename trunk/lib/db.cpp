/*
     Copyright (C) 2006-2007  Herve Fache

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

#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <dirent.h>
#include <errno.h>

using namespace std;

#include "files.h"
#include "dblist.h"
#include "db.h"
#include "hbackup.h"

using namespace hbackup;

struct Database::Private {
  DbList  active;
  DbList  removed;
};

int Database::organise(const string& path, int number) {
  DIR           *directory;
  struct dirent *dir_entry;
  File2         nofiles(path.c_str(), ".nofiles");
  int           failed   = 0;

  /* Already organised? */
  if (nofiles.isValid()) {
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
      if (file_data.type() == '?') {
        cerr << "db: organise: cannot get metadata: " << source_path << endl;
        failed = 2;
      } else
      if ((file_data.type() == 'd')
       // If we crashed, we might have some two-letter dirs already
       && (file_data.name().size() != 2)
       // If we've reached the point where the dir is ??-?, stop!
       && (file_data.name()[2] != '-')) {
        // Create two-letter directory
        string dir_path = path + "/" + file_data.name().substr(0,2);
        if (Directory("").create(dir_path.c_str())) {
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
      nofiles.create(path.c_str());
    }
  }
  closedir(directory);
  return failed;
}

int Database::write(
    const string&   path,
    char**          dchecksum,
    int             compress) {
  string    temp_path;
  string    dest_path;
  string    checksum;
  int       index = 0;
  int       deleteit = 0;
  int       failed = 0;

  Stream source(path.c_str());
  if (source.open("r")) {
    cerr << strerror(errno) << ": " << path << endl;
    return -1;
  }

  /* Temporary file to write to */
  temp_path = _path + "/filedata";
  Stream temp(temp_path.c_str());
  if (temp.open("w", compress)) {
    cerr << strerror(errno) << ": " << temp_path << endl;
    failed = -1;
  } else

  /* Copy file locally */
  if (temp.copy(source)) {
    cerr << strerror(errno) << ": " << path << endl;
    failed = -1;
  }

  source.close();
  temp.close();

  if (failed) {
    return failed;
  }

  /* Get file final location */
  if (getDir(source.checksum(), dest_path, true) == 2) {
    cerr << "db: write: failed to get dir for: " << source.checksum() << endl;
    failed = -1;
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
      checksum = string(source.checksum()) + "-" + str;
      final_path += str;
      if (! Directory("").create(final_path.c_str())) {
        /* Directory exists */
        File2 try_file(final_path.c_str(), "data");
        if (try_file.isValid()) {
          /* A file already exists, let's compare */
          File2 temp_md(temp_path.c_str());

          differ = (try_file.size() != temp_md.size());
        }
      }
      if (! differ) {
        dest_path = final_path;
        break;
      }
      index++;
    } while (true);

    /* Now move the file in its place */
    if (rename(temp_path.c_str(), (dest_path + "/data").c_str())) {
      cerr << "db: write: failed to move file " << temp_path
        << " to " << dest_path << ": " << strerror(errno);
      failed = -1;
    }
  }

  /* If anything failed, delete temporary file */
  if (failed || deleteit) {
    std::remove(temp_path.c_str());
  }

  // Report checksum
  *dchecksum = NULL;
  if (! failed) {
    asprintf(dchecksum, "%s", checksum.c_str());
  }

  // TODO Need to store compression data

  /* Make sure we won't exceed the file number limit */
  if (! failed) {
    /* dest_path is /path/to/checksum */
    unsigned int pos = dest_path.rfind('/');

    if (pos != string::npos) {
      dest_path.erase(pos);
      /* Now dest_path is /path/to */
      organise(dest_path, 256);
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
        std::remove(lock_path.c_str());
        status = 1;
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
  if (status != 2) {
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
  std::remove(lock_path.c_str());
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
    if (File2(path.c_str(), ".nofiles").isValid()) {
      path += "/" + checksum.substr(level, 2);
      level += 2;
      if (create && Directory("").create(path.c_str())) {
        return 1;
      }
    } else {
      break;
    }
  } while (true);
  // Return path
  path += "/" + checksum.substr(level);
  return ! Directory(path.c_str()).isValid();
}

int Database::open_removed() {
  return _d->removed.open(_path, "removed");
}

int Database::move_journals() {
  int status = 0;

  if (rename((_path + "/seen.journal").c_str(),
    (_path + "/seen").c_str())) {
    std::remove((_path + "/seen.journal").c_str());
    status |= 1;
  }
  if (rename((_path + "/gone.journal").c_str(),
    (_path + "/gone").c_str())) {
    std::remove((_path + "/gone.journal").c_str());
    status |= 1;
  }
  if (std::remove((_path + "/written.journal").c_str())) {
    status |= 1;
  }

  return status;
}

Database::Database(const string& path) {
  _path          = path;
  _expire_inited = false;
  _d             = new Private;
}

Database::~Database() {
  delete _d;
}

int Database::open() {
  int status;

  // Try to take lock
  int lock_status = lock();
  if (lock_status == 2) {
    errno = ENOLCK;
    return 2;
  }

  File2 active(_path.c_str(), "active");
  File2 removed(_path.c_str(), "removed");

  // Check that data dir exists, if not create it
  if (Directory(_path.c_str(), "data").isValid()) {
    status = 0;
    // Check files presence
    if (! active.isValid()) {
      cerr << "db: open: active files list not accessible: ";
      Stream backup(_path.c_str(), "active~");
      if (backup.isValid()) {
        cerr << "using backup" << endl;
        rename((_path + "/active~").c_str(), (_path + "/active").c_str());
      } else {
        cerr << "no backup accessible, aborting" << endl;
        status = 2;
      }
    }

    if ((status != 2) && ! removed.isValid()) {
      cerr << "db: open: removed files list not accessible: ";
      Stream backup(_path.c_str(), "removed~");
      if (backup.isValid()) {
        cerr << "using backup" << endl;
        rename((_path + "/removed~").c_str(), (_path + "/removed").c_str());
      } else {
        cerr << "no backup accessible, ignoring" << endl;
      }
    }
  } else if (! Directory(_path.c_str(), "data").create(_path.c_str())) {
    status = 1;
    // Create files
    if ((! active.isValid() && active.create(_path.c_str()))
     || (! removed.isValid() && removed.create(_path.c_str()))) {
      cerr << "db: open: cannot create list files" << endl;
      status = 2;
    } else if (verbosity() > 2) {
      cout << " --> Database initialized" << endl;
    }
  } else {
    status = 2;
    cerr << "db: open: cannot create data directory" << endl;
  }

  // Recover lists
  if ((status != 2) && File2(_path.c_str(), "written.journal").isValid()) {
    cerr << "db: open: journal found, attempting crash recovery" << endl;
    DbList  active;
    DbList  removed;

    // Add written and missing items to active list
    if (! active.load(_path, "written.journal") || (errno == EUCLEAN)) {
      cerr << "db: open: adding " << active.size()
        << " valid files to active list" << endl;

      // Get removed items journal
      removed.load(_path, "gone.journal");

      if ((errno == 0)
       && ! active.load(_path, "seen.journal", active.size())
       && ! active.load(_path, "active")) {
        cerr << "db: open: removing " << removed.size()
          << " files from active list" << endl;
        // Remove removed items from active list
        SortedList<DbData>::iterator i;
        for (i = removed.begin(); i != removed.end(); i++) {
          SortedList<DbData>::iterator j = active.find(*i);
          if (*i == *j) {
            active.erase(j);
          }
        }
        active.save(_path, "active");

        cerr << "db: open: new active list size: " << active.size() << endl;
      }
      active.clear();

      if ((removed.size() != 0) && ! removed.load(_path, "removed")) {
        removed.save(_path, "removed");
        cerr << "db: open: new removed list size: " << removed.size() << endl;
      }
      removed.clear();
    }

    // Move/delete journals
    move_journals();
  }

  // Make sure we start clean
  _d->active.clear();
  _d->removed.clear();

  // Read database active items list
  if (status != 2) {
    if (_d->active.open(_path, "active") == 2) {
      status = 2;
    }
  }

  if (status == 2) {
    unlock();
    return 2;
  }
  return 0;
}

int Database::close_active() {
  return _d->active.close(_path, "active");
}

int Database::close() {
  int status = 0;

  // Save active list
  if (_d->active.isOpen()) {
    if (_d->active.close(_path, "active") != 0) {
      return 2;
    }
  }

  // Save removed list
  if (_d->removed.size() != 0) {
    // Load previously removed items into removed list
    if (! _d->removed.isOpen()) {
      status = _d->removed.open(_path, "removed");
    }
  }
  if (_d->removed.isOpen() && (status != 2)) {
    status = _d->removed.close(_path, "removed");
  }

  // Delete journals
  move_journals();

  // Release lock
  unlock();
  return status;
}

int Database::expire_init() {
  if (! _d->active.isOpen()) {
    cerr << "db: cannot initialise expiration module unless active items lists"
    " is open" << endl;
    return 2;
  }
  SortedList<DbData>::iterator it;
  for (it = _d->active.begin(); it != _d->active.end(); it++) {
    if (it->checksum().size() != 0) {
      _active_checksums.push_back(it->checksum());
    }
  }
  _expire_inited = true;
  return 0;
}

int Database::expire_share(
    const string&   prefix,
    const string&   path,
    time_t          time_out) {
  if (! _expire_inited) {
    cerr << "db: expiration module not initialised" << endl;
    return 2;
  }
  if (! _d->removed.isOpen()) {
    cerr << "db: cannot expire unless removed items lists is open" << endl;
    return 2;
  }
  if (_d->removed.size() != 0) {
    SortedList<DbData>::iterator  it;
    time_t                        now = time(NULL);

    for (it = _d->removed.begin(); it != _d->removed.end(); it++) {
      // Prefix not reached
      if (it->prefix().substr(0, prefix.size()) < prefix) {
        continue;
      } else
      // Prefix exceeded
      if (it->prefix().substr(0, prefix.size()) > prefix) {
        break;
      } else
      // Prefix reached
      // Path not reached
      if (it->data()->path().substr(0, path.size()) < path) {
        continue;
      } else
      // Path exceeded
      if (it->data()->path().substr(0, path.size()) > path) {
        break;
      } else
      // Path reached
      {
        if ((now - it->out()) > time_out) {
          it->setExpired();
        } else {
          it->resetExpired();
        }
      }
    }
  }
  return 0;
}

int Database::expire_finalise() {
  if (! _expire_inited) {
    cerr << "db: expiration module not initialised" << endl;
    return 2;
  }
  if (! _d->removed.isOpen()) {
    cerr << "db: cannot expire unless removed items lists is open" << endl;
    return 2;
  }
  if (verbosity() > 1) {
    cout << " -> Removing expired records" << endl;
  }
  // Complete list of active records, create list of expired
  list<string> expired_checksums;
  SortedList<DbData>::iterator it;
  for (it = _d->removed.begin(); it != _d->removed.end();) {
    if (it->checksum().size() != 0) {
      if (it->expired()) {
        expired_checksums.push_back(it->checksum());
      } else {
        _active_checksums.push_back(it->checksum());
      }
    }
    if (it->expired()) {
      it = _d->removed.erase(it);
    } else {
      it++;
    }
  }
  // Sort and remove duplicates in each list
  expired_checksums.sort();
  expired_checksums.unique();
  if (verbosity() > 2) {
    cout << " --> Expired data: " << expired_checksums.size() << endl;
    if (verbosity() > 3) {
      list<string>::iterator it;
      for (it = expired_checksums.begin(); it != expired_checksums.end();
       it++) {
        cout << " ---> Checksum: " << *it << endl;
      }
    }
  }
  if (expired_checksums.size() != 0) {
    _active_checksums.sort();
    _active_checksums.unique();
    if (verbosity() > 2) {
      cout << " --> Active data : " << _active_checksums.size() << endl;
      if (verbosity() > 3) {
        list<string>::iterator it;
        for (it = _active_checksums.begin(); it != _active_checksums.end();
        it++) {
          cout << " ---> Checksum: " << *it << endl;
        }
      }
    }
    // Compare list to find expired that are not also in active list
    list<string>::iterator ex;
    list<string>::iterator ac = _active_checksums.begin();
    for (ex = expired_checksums.begin(); ex != expired_checksums.end();) {
      while ((ac != _active_checksums.end()) && (*ac < *ex)) {
        ac++;
      }
      if ((ac != _active_checksums.end()) && (*ac == *ex)) {
        // Removed from list
        ex = expired_checksums.erase(ex);
      } else {
        ex++;
      }
    }
    if (verbosity() > 2) {
      cout << " --> Erased data : " << expired_checksums.size() << endl;
    }
    list<string>::iterator it;
    for (it = expired_checksums.begin(); it != expired_checksums.end();
    it++) {
      if (verbosity() > 3) {
        cout << " ---> Checksum: " << *it << endl;
      }
      string path;
      if (getDir(*it, path, false) != 0) {
        cerr << "db: expired data not found" << endl;
      } else {
        // Remove data from DB
        std::remove((path + "/data").c_str());
        std::remove(path.c_str());
      }
    }
  }
  _expire_inited = false;
  return 0;
}

void Database::getList(
    const char*  prefix,
    const char*  base_path,
    const char*  rel_path,
    list<Node*>& list) {
  char* full_path = NULL;
  int length = asprintf(&full_path, "%s/%s/%s/", prefix, base_path, rel_path);
  if (rel_path[0] == '\0') {
    full_path[--length] = '\0';
  }

  // Look for beginning
  SortedList<DbData>::iterator entry = _d->active.begin();
  // Jump irrelevant first records
  while ((entry != _d->active.end())
      && (strncmp(entry->fullPath().c_str(), full_path, length) < 0)) {
    entry++;
  }
  // Copy relevant records
  char* last_dir     = NULL;
  int   last_dir_len = 0;
  while ((entry != _d->active.end())
      && (strncmp(entry->fullPath().c_str(), full_path, length) == 0)) {
    if ((last_dir == NULL)
     || strncmp(last_dir, entry->fullPath().c_str(), last_dir_len)) {
      Node* node;
      switch (entry->data()->type()) {
        case 'f':
          node = new File2(entry->data()->name().c_str(), entry->data()->type(),
            entry->data()->mtime(), entry->data()->size(),
            entry->data()->uid(), entry->data()->gid(), entry->data()->mode(),
            entry->checksum().c_str());
          break;
        case 'l':
          node = new Link(entry->data()->name().c_str(), entry->data()->type(),
            entry->data()->mtime(), entry->data()->size(),
            entry->data()->uid(), entry->data()->gid(), entry->data()->mode(),
            entry->data()->link().c_str());
          break;
        default:
          node = new Node(entry->data()->name().c_str(), entry->data()->type(),
            entry->data()->mtime(), entry->data()->size(),
            entry->data()->uid(), entry->data()->gid(), entry->data()->mode());
      }
      if (node->type() == 'd') {
        free(last_dir);
        last_dir = NULL;
        last_dir_len = asprintf(&last_dir, "%s%s/", full_path, node->name());
      }
      list.push_back(node);
    }
    entry++;
  }
  free(last_dir);
  free(full_path);
}

int Database::parse(
    const string& prefix,
    const string& mounted_path,
    const string& mount_path,
    list<File>*   file_list) {
  int     failed        = 0;
  int     removed_size  = _d->removed.size();
  DbList  added;

  string full_path = prefix + "/" + mounted_path + "/";
  SortedList<DbData>::iterator entry_active = _d->active.begin();
  // Jump irrelevant first records
  while ((entry_active != _d->active.end())
   && (entry_active->fullPath(full_path.size()) < full_path)) {
    entry_active++;
  }
  SortedList<DbData>::iterator active_last = _d->active.end();

  list<File>::iterator          entry_file_list = file_list->begin();
  while (! terminating()) {
    bool file_add  = false;
    bool db_remove = false;
    bool same_file = false;

    // Check whether db data is (still) relevant
    if ((entry_active != _d->active.end()) && (active_last != entry_active)
     && (entry_active->fullPath(full_path.size()) > full_path)) {
      // Irrelevant rest of list
      entry_active = _d->active.end();
    }
    active_last = entry_active;

    // Check whether we have more work to do
    if ((entry_active == _d->active.end())
     && (entry_file_list == file_list->end())) {
      break;
    }

    // Deal with each case
    if (entry_active == _d->active.end()) {
      // End of db relevant data reached: add element
      file_add = true;
    } else
    if (entry_file_list == file_list->end()) {
      // End of file data reached: remove element
      db_remove = true;
    } else {
      int result = entry_active->data()->path().substr(mounted_path.size() + 1).compare(entry_file_list->path());

      if (result < 0) {
        db_remove = true;
      } else
      if (result > 0) {
        file_add = true;
      } else
      if (entry_active->data()->metadiffer(*entry_file_list)) {
        db_remove = true;
        file_add = true;

        /* If it's a file and size and mtime are the same, copy checksum accross */
        if ((entry_active->data()->type() == 'f')
         && (entry_active->data()->size() == entry_file_list->size())
         && (entry_active->data()->mtime() == entry_file_list->mtime())) {
          // Same file, just ownership changed or something
          same_file = true;
         }
      } else
      if ((entry_active->data()->type() == 'f')
       && (entry_active->checksum().size() == 0)) {
        // Previously failed write
        db_remove = true;
        file_add = true;
      } else {
        entry_active++;
        entry_file_list++;
      }
    }

    if (file_add) {
      // Create new record
      File file_data(*entry_file_list);
      file_data.setPath(mounted_path + "/" + entry_file_list->path());
      DbData db_data(file_data);
      db_data.setPrefix(prefix);
      if (same_file) {
        db_data.setChecksum(entry_active->checksum());
      }
      // Add to added list
      added.add(db_data);
      // Go on to next
      entry_file_list++;
    }

    // DB data is used above!
    if (db_remove) {
      // Mark removed
      entry_active->setOut();
      // Append to removed list
      _d->removed.push_back(*entry_active);
      // Remove from active list / Go on to next
      entry_active = _d->active.erase(entry_active);
    }
  }

  // Append to recovery journals
  if (added.save_journal(_path, "seen.journal")
   || _d->removed.save_journal(_path, "gone.journal", removed_size)) {
    // Unlikely
    failed = 1;
  }

  // Deal with new/modified data
  if (! added.empty()) {
    // Static to be global to all shares
    long long sizetobackup  = 0;
    long long sizebackedup  = 0;
    int       filestobackup = 0;
    int       filesbackedup = 0;
    SortedList<DbData>::iterator i;

    // Determine volume to be copied
    if (verbosity() > 2) {
      for (i = added.begin(); i != added.end(); i++) {
        /* Same data as in file_list */
        filestobackup++;
        if ((i->data()->type() == 'f') && i->checksum().empty()) {
          sizetobackup += i->data()->size();
        }
      }
      printf(" --> Files to add: %u (%lld bytes)\n",
        added.size(), sizetobackup);
    }

    // Create recovery journal for added files
    FILE  *writefile;
    bool  journal_ok = true;

    string dest_path = _path + "/written.journal";
    if ((writefile = fopen(dest_path.c_str(), "a")) == NULL) {
      journal_ok = false;
    }

    for (i = added.begin(); i != added.end(); i++) {
      if (terminating()) {
        break;
      }
      // Set checksum
      if ((i->data()->type() == 'f') && i->checksum().empty()) {
        char* checksum = NULL;
        if (write(mount_path + i->data()->path().substr(mounted_path.size()),
         &checksum) != 0) {
          /* Write failed, need to go on */
          i->setChecksum("");
          failed = 1;
        } else {
          i->setChecksum(checksum);
          free(checksum);
        }
        if (verbosity() > 2) {
          sizebackedup += i->data()->size();
        }
      }
      _d->active.add(*i);
      if (journal_ok) {
        // Update journal
        fprintf(writefile, "%s\n", i->line().c_str());
      }

      // Update/show information
      if (verbosity() > 2) {
        filesbackedup++;
        if (sizetobackup != 0) {
          printf(" --> Done: %d/%d (%.1f%%)\r", filesbackedup, filestobackup,
            100.0 * sizebackedup / sizetobackup);
        } else {
          printf(" --> Done: %d/%d\r", filesbackedup, filestobackup);
        }
        fflush(stdout);
      }
    }
    if (journal_ok) {
      fclose(writefile);
    }
    if ((verbosity() > 2) && (filesbackedup != 0.0)) {
      cout << endl;
    }
  } else if (verbosity() > 2) {
    cout << " --> Files to add: 0" << endl;
  }

  // Deal with removed/modified data
  if (verbosity() > 2) {
    cout << " --> Files to remove: " << _d->removed.size() - removed_size << endl;
  }

  // Report errors
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
  Stream source(source_path.c_str());
  if (source.open("r")) {
    cerr << "db: read: failed to open source file: " << source_path << endl;
    return 2;
  }
  Stream temp(temp_path.c_str());
  if (temp.open("w")) {
    cerr << "db: read: failed to open dest file: " << temp_path << endl;
    failed = 2;
  } else

  if (temp.copy(source)) {
    cerr << "db: read: failed to copy file: " << source_path << endl;
    failed = 2;
  }

  source.close();
  temp.close();

  if (! failed) {
    /* Verify that checksums match before overwriting final destination */
    if (strncmp(source.checksum(), temp.checksum(), strlen(temp.checksum()))) {
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
  }

  if (failed) {
    std::remove(temp_path.c_str());
  }

  return failed;
}

int Database::scan(const string& checksum, bool thorough) {
  int failed = 0;

  if (checksum == "") {
    int files = _d->active.size();

    if (verbosity() > 2) {
      cout << " --> Scanning database contents";
      if (thorough) {
        cout << " thoroughly";
      }
      cout << ": " << _d->active.size() << " file";
      if (_d->active.size() != 1) {
        cout << "s";
      }
      cout << endl;
    }

    SortedList<DbData>::iterator i;
    for (i = _d->active.begin(); i != _d->active.end(); i++) {
      if ((verbosity() > 2) && ((files & 0xFF) == 0)) {
        printf(" --> Files left to go: %u\n", files);
      }
      files--;
      if ((! i->checksum().empty()) && scan(i->checksum(), thorough)) {
        i->setChecksum("");
        failed = 1;
        if (! terminating() && verbosity() > 2) {
          struct tm *time;
          cout << " --> Client:      " << i->prefix() << endl;
          cout << " --> File name:   " << i->data()->path() << endl;
          if (verbosity() > 3) {
            time_t file_mtime = i->data()->mtime();
            time = localtime(&file_mtime);
            printf(" --> Modified:    %04u-%02u-%02u %2u:%02u:%02u %s\n",
              time->tm_year + 1900, time->tm_mon + 1, time->tm_mday,
              time->tm_hour, time->tm_min, time->tm_sec, time->tm_zone);
              time_t local = i->in();
              time = localtime(&local);
            printf(" --> Seen first: %04u-%02u-%02u %2u:%02u:%02u %s\n",
              time->tm_year + 1900, time->tm_mon + 1, time->tm_mday,
              time->tm_hour, time->tm_min, time->tm_sec, time->tm_zone);
            if (i->out() != 0) {
              time_t local = i->out();
              time = localtime(&local);
              printf(" --> Seen gone:  %04u-%02u-%02u %2u:%02u:%02u %s\n",
                time->tm_year + 1900, time->tm_mon + 1, time->tm_mday,
                time->tm_hour, time->tm_min, time->tm_sec, time->tm_zone);
            }
          }
        }
      }
      if (terminating()) {
        failed = 3;
        break;
      }
    }
  } else {
    string  path;
    bool    filefailed = false;

    if (getDir(checksum, path, false)) {
      errno = ENODATA;
      filefailed = true;
      cerr << "db: scan: failed to get directory for checksum " << checksum << endl;
    } else
    if (! File2(path.c_str(), "data").isValid()) {
      errno = ENOENT;
      filefailed = true;
      cerr << "db: scan: file data missing for checksum " << checksum << endl;
    } else
    if (thorough) {
      string check_path = path + "/data";

      /* Read file to compute checksum, compare with expected */
      Stream s(check_path.c_str());
      if (s.computeChecksum()) {
        errno = ENOENT;
        filefailed = true;
        cerr << "db: scan: file data missing for checksum " << checksum << endl;
      } else
      if (strncmp(checksum.c_str(), s.checksum(), strlen(s.checksum()))) {
        errno = EILSEQ;
        filefailed = true;
        if (! terminating()) {
          cerr << "db: scan: file data corrupted for checksum " << checksum
            << " (found to be " << s.checksum() << ")" << endl;
        }
      }

      // Remove corrupted file if any
      if (filefailed) {
        std::remove(check_path.c_str());
      }
    }
    if (filefailed) {
      failed = 1;
    }
  }
  return failed;
}

int Database::add(
    const char* prefix,
    const char* base_path,
    const char* rel_path,
    const char* dir_path,
    const Node* node) {
  // Add new record to active list
  if ((node->type() == 'l') && ! node->parsed()) {
    cerr << "Bug in db add: link is not parsed!" << endl;
    return -1;
  }

  // Determine path
  string full_path = base_path;
  if (rel_path[0] != '\0') {
    full_path += string("/") + rel_path;
  }
  full_path += string("/") + node->name();

  // Create data
  DbData data(
    File(
      full_path, (node->type() == 'l') ? ((Link*)node)->link() : "",
      node->type(), node->mtime(), node->size(),
      node->uid(), node->gid(), node->mode()));
  data.setPrefix(prefix);

  // Copy data
  if (node->type() == 'f') {
    char* local_path = NULL;
    char* checksum   = NULL;
    asprintf(&local_path, "%s/%s", dir_path, node->name());
    if (! write(string(local_path), &checksum)) {
      data.setChecksum(checksum);
      free(checksum);
    }
    free(local_path);
  }

  // Insert entry
  _d->active.add(data);
  return 0;
}

int Database::modify(
    const char* prefix,
    const char* base_path,
    const char* rel_path,
    const char* dir_path,
    const Node* node,
    bool        no_data) {
#warning modify not implemented
  return 0;
}

int Database::remove(
    const char* prefix,
    const char* base_path,
    const char* rel_path,
    const Node* node) {
  // Find record in active list, move it to removed list
  char* full_path = NULL;
  int length;
  if (rel_path[0] != '\0') {
    length = asprintf(&full_path, "%s/%s/%s/%s", prefix, base_path, rel_path,
      node->name());
  } else {
    length = asprintf(&full_path, "%s/%s/%s", prefix, base_path, node->name());
  }

  SortedList<DbData>::iterator entry = _d->active.begin();
  // Jump irrelevant first records
  int cmp = -1;
  while ((entry != _d->active.end())
      && ((cmp = strncmp(entry->fullPath().c_str(), full_path, length)) < 0)) {
    entry++;
  }
  while ((entry != _d->active.end())
      && (strncmp(entry->fullPath().c_str(), full_path, length) == 0)) {
    // Mark removed
    entry->setOut();
    // Append to removed list
    _d->removed.push_back(*entry);
    // Remove from active list / Go on to next
    entry = _d->active.erase(entry);
  }
  free(full_path);
  return 0;
}

// For debug only
void* Database::active() {
  return &_d->active;
}

void* Database::removed() {
  return &_d->removed;
}
