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

#include <fstream>
#include <iostream>
#include <vector>
#include <list>
#include <string>
#include <errno.h>

using namespace std;

#include "files.h"
#include "list.h"
#include "filters.h"
#include "parsers.h"
#include "cvs_parser.h"
#include "paths.h"
#include "db.h"
#include "hbackup.h"
#include "clients.h"

int Client::mountPath(
    string        backup_path,
    string        *path) {
  string command = "mount ";
  string share;

  /* Determine share and path */
  if (_protocol == "file") {
    share = "";
    *path  = backup_path;
  } else
  if (_protocol == "nfs") {
    share = _host_or_ip + ":" + backup_path;
    *path  = _mount_point;
  } else
  if (_protocol == "smb") {
    share = "//" + _host_or_ip + "/" + backup_path.substr(0,1) + "$";
    *path  = _mount_point + "/" +  backup_path.substr(3);
  } else {
    errno = EPROTONOSUPPORT;
    return 2;
  }

  /* Check what is mounted */
  if (_mounted != "") {
    if (_mounted != share) {
      /* Different share mounted: unmount */
      umount();
    } else {
      /* Same share mounted: nothing to do */
      return 0;
    }
  }

  /* Build mount command */
  if (_protocol == "file") {
    return 0;
  } else
  if (_protocol == "nfs") {
    command += "-t nfs -o ro,noatime,nolock";
  } else
  if (_protocol == "smb") {
    // codepage=858
    command += "-t cifs -o ro,noatime,nocase";
  }
  // Additional options
  for (unsigned int i = 0; i < _options.size(); i++ ) {
    command += "," + _options[i]->option();
  }
  // Paths
  command += " " + share + " " + _mount_point;

  /* Issue mount command */
  if (verbosity() > 3) {
    cout << " ---> " << command << endl;
  }
  command += " > /dev/null 2>&1";
  int result = system(command.c_str());
  if (result != 0) {
    errno = ETIMEDOUT;
  } else {
    _mounted = share;
  }
  return result;
}

int Client::umount() {
  if (_mounted != "") {
    string command = "umount ";

    command += _mount_point;
    if (verbosity() > 3) {
      cout << " ---> " << command << endl;
    }
    _mounted = "";
    return system(command.c_str());
  }
  return 0;
}

int Client::readListFile(const string& list_path) {
  string  buffer;
  int     line    = 0;
  int     failed  = 0;

  /* Open list file */
  ifstream list_file(list_path.c_str());

  /* Open list file */
  if (! list_file.is_open()) {
//     cerr << "List file not found " << list_path << endl;
    failed = 2;
  } else {
    if (verbosity() > 1) {
      printf(" -> Reading backup list file\n");
    }

    /* Read list file */
    Path* path = NULL;
    while (! list_file.eof() && ! failed) {
      getline(list_file, buffer);
      unsigned int pos = buffer.find("\r");
      if (pos != string::npos) {
        buffer.erase(pos);
      }
      vector<string> params;

      line++;
      if (File::decodeLine(buffer, params)) {
        errno = EUCLEAN;
        cerr << "Warning: in list file " << list_path << ", line " << line
          << " missing single- or double-quote" << endl;
      }
      if (params.size() >= 1) {
        if (params[0] == "path") {
          // Expect exactly two parameters
          if (params.size() != 2) {
            cerr << "Error: in list file " << list_path << ", line " << line
              << " 'path' takes exactly one argument" << endl;
            failed = 1;
          } else
          /* New backup path */
          path = new Path(params[1]);
          if (verbosity() > 2) {
            cout << " --> Path: " << path->path() << endl;
          }
          _paths.push_back(path);
        } else if (path != NULL) {
          if ((params[0] == "ignore") || (params[0] == "ignand")) {
            // Expect exactly three parameters
            if (params.size() != 3) {
              cerr << "Error: in list file " << list_path << ", line " << line
                << " 'filter' takes exactly two arguments" << endl;
              failed = 1;
            } else
            if (path->addFilter(params[1], params[2], params[0] == "ignand")) {
              cerr << "Error: in list file " << list_path << ", line " << line
                << " unsupported filter: " << params[1] << endl;
              failed = 1;
            }
          } else
          if (params[0] == "parser") {
            // Expect exactly three parameters
            if (params.size() != 3) {
              cerr << "Error: in list file " << list_path << ", line " << line
                << " 'parser' takes exactly two arguments" << endl;
              failed = 1;
            } else
            if (path->addParser(params[1], params[2])) {
              cerr << "Error: in list file " << list_path << ", line " << line
                << " unsupported parser: " << params[2] << endl;
              failed = 1;
            }
          } else {
            // What was that?
            if (params.size() != 2) {
              cerr << "Error: in list file " << list_path << ", line " << line
                << " unknown keyword: " << params[0] << endl;
              failed = 1;
            }
          }
        } else {
          // What?
          cerr << "Error: in list file " << list_path << ", line " << line
            << " unexpected keyword at this level: " << params[0] << endl;
          failed = 1;
        }
      }
    }
    /* Close list file */
    list_file.close();
  }
  return failed;
}

Client::Client(string value) {
  _name         = value;
  _host_or_ip   = _name;
  _listfilename = "";
  _listfiledir  = "";
  _protocol     = "";
  _mount_point  = "";
  _mounted      = "";

  if (verbosity() > 2) {
    cout << " --> Client: " << _name << endl;
  }
}

Client::~Client() {
  for (unsigned int i = 0; i < _options.size(); i++) {
    delete _options[i];
  }
  for (unsigned int i = 0; i < _paths.size(); i++) {
    delete _paths[i];
  }
}

void Client::setHostOrIp(string value) {
  _host_or_ip = value;
}

void Client::setProtocol(string value) {
  _protocol = value;
}

void Client::setListfile(string value) {
  // Convert to UNIX style for paths
  Path path(value);
  unsigned int pos = path.path().rfind('/');
  if ((pos != string::npos) && (pos < path.path().size()) && (pos > 0)) {
    _listfilename = path.path().substr(pos + 1);
    _listfiledir  = path.path().substr(0, pos);
  } else {
    _listfilename = "";
    _listfiledir  = "";
  }
}

int Client::backup(
    Database& db,
    bool      config_check) {
  int     failed = 0;
  int     clientfailed  = 0;
  string  share;
  string  list_path;

  if (mountPath(_listfiledir, &list_path)) {
    switch (errno) {
      case EPROTONOSUPPORT:
        cerr << "Protocol not supported: " << _protocol << endl;
        return 1;
      case ETIMEDOUT:
        if (verbosity() > 0) {
          cout << "Client unreachable '" << _name
            << "' using protocol '" << _protocol << "'" << endl;
        }
        return 0;
    }
  }
  list_path += "/" + _listfilename;

  if (verbosity() > 0) {
    cout << "Backup client '" << _name
      << "' using protocol '" << _protocol << "'" << endl;
  }

  if (! readListFile(list_path)) {
    /* Backup */
    if (_paths.empty()) {
      failed = 1;
    } else if (! config_check) {
      for (unsigned int i = 0; i < _paths.size(); i++) {
        if (terminating() || clientfailed) {
          break;
        }
        string  backup_path;

        if (verbosity() > 0) {
          cout << "Backup path '" << _paths[i]->path() << "'" << endl;
          if (verbosity() > 1) {
            cout << " -> Building list of files" << endl;
          }
        }

        if (mountPath(_paths[i]->path(), &backup_path)) {
          cerr << "clients: backup: mount failed for "
            << _paths[i]->path() << endl;
          failed = 1;
        } else
        if (_paths[i]->createList(backup_path)) {
          // prepare_share sets errno
          if (! terminating()) {
            cerr << "clients: backup: list creation failed" << endl;
          }
          failed        = 1;
          clientfailed  = 1;
        } else {
          if (verbosity() > 1) {
            cout << " -> Parsing list of files ("
              << _paths[i]->list()->size() << ")" << endl;
          }
          if (db.parse(_protocol + "://" + _name, _paths[i]->path(),
            backup_path, _paths[i]->list())) {
            failed        = 1;
          }
          _paths[i]->clearList();
        }
      }
    }
  } else {
    // errno set by functions called
    switch (errno) {
      case ENOENT:
        cerr << "List file not found " << list_path << endl;
        break;
      case EUCLEAN:
        cerr << "List file corrupted " << list_path << endl;
    }
    failed = 1;
  }
  umount(); // does not change errno
  return failed;
}

void Client::show() {
  cout << "-> " << _protocol << "://" << _host_or_ip << " "
    << _listfiledir << "/" << _listfilename << endl;
  if (_options.size() > 0) {
    cout << "Options:";
    for (unsigned int i = 0; i < _options.size(); i++ ) {
      cout << " " + _options[i]->option();
    }
    cout << endl;
  }
}
