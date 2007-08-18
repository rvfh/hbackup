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

#warning journals gone

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

  // Release lock
  unlock();
  return status;
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

  // TODO Look for exact match until order is fixed ('.', '-' are before '/')
  // Look for beginning
  SortedList<DbData>::iterator entry = _d->active.begin();
  // Jump irrelevant first records
  while ((entry != _d->active.end())
      && (entry->comparePath(full_path, length) != 0)) {
    entry++;
  }
  // Copy relevant records
  char* last_dir     = NULL;
  int   last_dir_len = 0;
  while ((entry != _d->active.end())
      && (entry->comparePath(full_path, length) == 0)) {
    if ((last_dir == NULL) || entry->comparePath(last_dir, last_dir_len)) {
      Node* node;
      switch (entry->data()->type()) {
        case 'f':
          node = new File2(*((File2*) entry->data()));
          break;
        case 'l':
          node = new Link(*((Link*) entry->data()));
          break;
        default:
          node = new Node(*entry->data());
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
      if (i->data()->type() == 'f') {
        File2* f = (File2*) i->data();
        if ((f->checksum()[0] != '\0') && scan(f->checksum(), thorough)) {
#warning need to signal problem in DB list
          failed = 1;
          if (! terminating() && verbosity() > 2) {
            struct tm *time;
            cout << " --> Client:      " << i->prefix() << endl;
            cout << " --> File:        " << i->path() << endl;
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
    const Node* node,
    const char* old_checksum) {
  // Add new record to active list
  if ((node->type() == 'l') && ! node->parsed()) {
    cerr << "Bug in db add: link is not parsed!" << endl;
    return -1;
  }

  // Determine path
  char* full_path = NULL;
  if (rel_path[0] != '\0') {
    asprintf(&full_path, "%s/%s/%s", base_path, rel_path, node->name());
  } else {
    asprintf(&full_path, "%s/%s", base_path, node->name());
  }

  // Create data
  Node* node2;
  switch (node->type()) {
    case 'l':
      node2 = new Link(*(Link*)node);
      break;
    case 'f':
      node2 = new File2(*node);
      if (old_checksum != NULL) {
        // Use same checksum
        ((File2*)node2)->setChecksum(old_checksum);
      } else {
        // Copy data
        char* local_path = NULL;
        char* checksum   = NULL;
        asprintf(&local_path, "%s/%s", dir_path, node->name());
        if (! write(string(local_path), &checksum)) {
          ((File2*)node2)->setChecksum(checksum);
          free(checksum);
        }
        free(local_path);
      }
      break;
    default:
      node2 = new Node(*node);
  }
  DbData data(prefix, full_path, node2);
  free(full_path);

  // Insert entry
  _d->active.add(data);
  return 0;
}

int Database::modify(
    const char* prefix,
    const char* base_path,
    const char* rel_path,
    const char* dir_path,
    const Node* old_node,
    const Node* node,
    bool        no_data) {
  if (! no_data) {
    if (add(prefix, base_path, rel_path, dir_path, node, NULL)) {
      return -1;
    }
  } else if (((File2*)old_node)->checksum()[0] == '\0') {
      // File is in the list, but could not be copied last time, try again
#warning retry not implemented
    return -1;
  } else {
    // File metadata has changed, but we believe the data is the same
    if (add(prefix, base_path, rel_path, dir_path, node,
            ((File2*)old_node)->checksum())) {
      return -1;
    }
  }
  remove(prefix, base_path, rel_path, old_node, false);
  return 0;
}

void Database::remove(
    const char* prefix,
    const char* base_path,
    const char* rel_path,
    const Node* node,
    bool        descend) {
  // Find record in active list, move it to removed list
  char* full_path = NULL;
  int length;
  if (rel_path[0] != '\0') {
    length = asprintf(&full_path, "%s/%s/%s/%s/", prefix, base_path, rel_path,
      node->name());
  } else {
    length = asprintf(&full_path, "%s/%s/%s/", prefix, base_path,
      node->name());
  }
  // Do not use trailing '/' for now
  full_path[length - 1] = '\0';

  SortedList<DbData>::iterator entry = _d->active.begin();
  // Jump irrelevant first records
  int cmp = -1;
  while ((entry != _d->active.end())
      && ((cmp = entry->comparePath(full_path)) < 0)) {
    entry++;
  }
  if ((entry != _d->active.end()) && (cmp == 0)) {
    // Mark removed
    entry->setOut();
    // Append to removed list
    _d->removed.push_back(*entry);
    // Remove from active list / Go on to next
    entry = _d->active.erase(entry);

    if (descend && (node->type() == 'd')) {
      // Now we need the trailing '/'
      full_path[length - 1] = '/';
      while ((entry != _d->active.end())
          && (entry->comparePath(full_path, length) == 0)) {
        // Mark removed
        entry->setOut();
        // Append to removed list
        _d->removed.push_back(*entry);
        // Remove from active list / Go on to next
        entry = _d->active.erase(entry);
      }
    }
  }
  free(full_path);
}

// For debug only
void* Database::active() {
  return &_d->active;
}

void* Database::removed() {
  return &_d->removed;
}
