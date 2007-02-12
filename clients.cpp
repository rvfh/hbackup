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

using namespace std;

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <errno.h>

#include "metadata.h"
#include "common.h"
#include "params.h"
#include "tools.h"
#include "list.h"
#include "filters.h"
#include "parsers.h"
#include "filelist.h"
#include "cvs_parser.h"
#include "db.h"
#include "clients.h"

typedef struct {
  string  path;
  Filter  *ignore_handle;
  List    *parsers_handle;
} path_t;

static string mounted = "";

int Client::unmount_share(const string& mount_point) {
  if (mounted != "") {
    string command = "umount ";

    command += mount_point;
    mounted = "";
    return system(command.c_str());
  }
  return 0;
}

int Client::mount_share(const string& mount_point, const string& client_path) {
  int    result   = 0;
  string share;
  string command = "mount ";

  errno = 0;
  /* Determine share */
  if (_protocol == "file") {
    share = "";
  } else
  if (_protocol == "nfs") {
    /* Mount Network File System share (host_or_ip:share) */
    share = _host_or_ip + ":" + client_path;
  } else
  if (_protocol == "smb") {
    /* Mount SaMBa share (//host_or_ip/share) */
    share = "//" + _host_or_ip + "/" + client_path;
  }

  /* Check what is mounted */
  if (mounted != "") {
    if (mounted != share) {
      /* Different share mounted: unmount */
      unmount_share(mount_point);
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
    command += "-t nfs -o ro,noatime,nolock "+ share + " " + mount_point;
  } else
  if (_protocol == "smb") {
    // codepage=858
    command += "-t cifs -o ro,noatime,nocase";

    for (unsigned int i = 0; i < _options.size(); i++ ) {
      command += "," + _options[i].option();
    }
    command += " " + share + " " + mount_point + " > /dev/null 2>&1";
  }

  /* Issue mount command */
  if (verbosity() > 2) {
    cout << " --> " << command << endl;
  }
  result = system(command.c_str());
  if (result != 0) {
    errno = ETIMEDOUT;
  } else {
    mounted = share;
  }
  return result;
}

Client::Client(string value) {
  _name     = value;
  if (verbosity() > 2) {
    cout << " --> Client: " << _name << endl;
  }
}

void Client::setHostOrIp(string value) {
  _host_or_ip = value;
}

void Client::setProtocol(string value) {
  _protocol = string(value);
}

void Client::setListfile(string value) {
  _listfile = string(value);
}

void Client::show() {
  cout << "-> " << _protocol << "://";
  if (_host_or_ip != "") {
    cout << _host_or_ip;
  } else {
    cout << "localhost";
  }
  cout << " " << _listfile << endl;
  if (_options.size() > 0) {
    cout << "Options:";
    for (unsigned int i = 0; i < _options.size(); i++ ) {
      cout << " " + _options[i].option();
    }
    cout << endl;
  }
}

static int add_filter(Filter *handle, const char *type, const char *string) {
  const char *filter_type;
  const char *delim    = strchr(type, '/');
  mode_t     file_type = 0;

  /* Check whether file type was specified */
  if (delim != NULL) {
    filter_type = delim + 1;
    if (! strncmp(type, "file", delim - type)) {
      file_type = S_IFREG;
    } else if (! strncmp(type, "dir", delim - type)) {
      file_type = S_IFDIR;
    } else if (! strncmp(type, "char", delim - type)) {
      file_type = S_IFCHR;
    } else if (! strncmp(type, "block", delim - type)) {
      file_type = S_IFBLK;
    } else if (! strncmp(type, "pipe", delim - type)) {
      file_type = S_IFIFO;
    } else if (! strncmp(type, "link", delim - type)) {
      file_type = S_IFLNK;
    } else if (! strncmp(type, "socket", delim - type)) {
      file_type = S_IFSOCK;
    } else {
      return 1;
    }
  } else {
    filter_type = type;
    file_type = S_IFMT;
  }

  /* Add specified filter */
  if (! strcmp(filter_type, "path_end")) {
    handle->push_back(new Rule(new Condition(file_type, filter_path_end, string)));
  } else if (! strcmp(filter_type, "path_start")) {
    handle->push_back(new Rule(new Condition(file_type, filter_path_start, string)));
  } else if (! strcmp(filter_type, "path_regexp")) {
    handle->push_back(new Rule(new Condition(file_type, filter_path_regexp, string)));
  } else if (! strcmp(filter_type, "size_below")) {
    handle->push_back(new Rule(new Condition(0, filter_size_below, strtoul(string, NULL, 10))));
  } else if (! strcmp(filter_type, "size_above")) {
    handle->push_back(new Rule(new Condition(0, filter_size_above, strtoul(string, NULL, 10))));
  } else {
    return 1;
  }
  return 0;
}

static int add_parser(List *handle, const char *type, const char *string) {
  parser_mode_t mode;

  /* Determine mode */
  if (! strcmp(type, "mod")) {
    mode = parser_modified;
  } else if (! strcmp(type, "mod+oth")) {
    mode = parser_modifiedandothers;
  } else if (! strcmp(type, "oth")) {
    mode = parser_others;
  } else {
    /* Default */
    mode = parser_controlled;
  }

  /* Add specified parser */
  if (! strcmp(string, "cvs")) {
    parsers_add(handle, mode, cvs_parser_new());
  } else {
    return 1;
  }
  return 0;
}

static int read_listfile(const string& list_path, List *backups) {
  string  buffer;
  int     line    = 0;
  path_t  *backup = NULL;
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
    while (! list_file.eof() && ! failed) {
      getline(list_file, buffer);
      unsigned int pos = buffer.find("\r");
      if (pos != string::npos) {
        buffer.erase(pos);
      }
      char    keyword[256];
      char    type[256];
      string  value;
      int     params = params_readline(buffer, keyword, type, &value);

      line++;
      if (params <= 0) {
        if (params < 0) {
          errno = EUCLEAN;
          cerr << "clients: backup: syntax error in list file "
            << list_path << " line " << line << endl;
          failed = 1;
        }
      } else if (! strcmp(keyword, "ignore")) {
        if (add_filter(backup->ignore_handle, type, value.c_str())) {
          cerr << "clients: backup: unsupported filter in list file "
            << list_path << " line " << line << endl;
        }
      } else if (! strcmp(keyword, "parser")) {
        strtolower(value);
        if (add_parser(backup->parsers_handle, type, value.c_str())) {
          cerr << "clients: backup: unsupported parser '" << value
            << "'in list file " << list_path << " line " << line << endl;
        }
      } else if (! strcmp(keyword, "path")) {
        /* New backup entry */
        backup = new path_t;
        backup->ignore_handle = new Filter;
        parsers_new(&backup->parsers_handle);
        backup->path = value;
        no_trailing_slash(backup->path);
        if (verbosity() > 2) {
          cout << " --> Path: " << backup->path << endl;
        }
        backups->append(backup);
      } else {
        errno = EUCLEAN;
        cerr << "clients: backup: syntax error in list file "
          << list_path << " line " << line << endl;
        failed = 1;
      }
    }
    /* Close list file */
    list_file.close();
  }
  return failed;
}

/* Return 1 to force mount */
static int get_paths(
    string  protocol,
    string  backup_path,
    string  mount_point,
    string  *share,
    string  *path) {
  if (protocol == "file") {
    *share = "";
    *path  = backup_path;
    errno  = 0;
    return 0;
  } else

  if (protocol == "nfs") {
    *share = backup_path;
    *path  = mount_point;
    errno  = 0;
    return 1;
  } else

  if (protocol == "smb") {
    *share = backup_path.substr(0,1) + "$";
    strtolower(*share);
    *path  = mount_point + "/" +  backup_path.substr(3);
    /* Lower case */
    strtolower(*path);
    pathtolinux(*path);
    errno = 0;
    return 1;
  }

  errno = EPROTONOSUPPORT;
  return 2;
}

int Client::backup(
    string  mount_point,
    bool    configcheck) {
  int     failed = 0;
  List    *backups      = new List();
  int     clientfailed  = 0;
  string  share;
  string  list_path;

  switch (get_paths(_protocol, _listfile, mount_point, &share, &list_path)) {
    case 1:
      mount_share(mount_point, share);
      break;
  }
  if (errno != 0) {
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

  if (verbosity() > 0) {
    cout << "Backup client '" << _name
      << "' using protocol '" << _protocol << "'" << endl;
  }

  if (! read_listfile(list_path, backups)) {
    /* Backup */
    if (backups->size() == 0) {
      failed = 1;
    } else if (! configcheck) {
      list_entry_t *entry = NULL;

      while (((entry = backups->next(entry)) != NULL) && ! clientfailed) {
        path_t  *backup = (path_t *) (list_entry_payload(entry));
        string  backup_path;

        /* TODO Stop when terminating (=> backup destructor) */
        if (verbosity() > 0) {
          cout << "Backup path '" << backup->path << "'" << endl;
          if (verbosity() > 1) {
            cout << " -> Building list of files" << endl;
          }
        }

        switch (get_paths(_protocol, backup->path, mount_point, &share, &backup_path)) {
          case 1:
            if (mount_share(mount_point, share)) {
              clientfailed = 1;
            }
            break;
          case 2:
            clientfailed = 1;
        }

        if ( ! clientfailed
          && ! filelist_new(backup_path.c_str(), backup->ignore_handle,
            backup->parsers_handle)) {
          string prefix;

          if (verbosity() > 1) {
            printf(" -> Parsing list of files\n");
          }
          prefix = _protocol + "://" + _name;
          strtolower(backup->path);
          pathtolinux(backup->path);
          if (db_parse(prefix, backup->path, backup_path, filelist_get())) {
            failed        = 1;
          }
          filelist_free();
        } else {
          // prepare_share sets errno
          if (! terminating()) {
            fprintf(stderr, "clients: backup: list creation failed\n");
          }
          failed        = 1;
          clientfailed  = 1;
        }

        /* Free encapsulated data */
        delete backup->ignore_handle;
        parsers_free(backup->parsers_handle);
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
  /* Free backups list */
  delete backups;
  unmount_share(mount_point); // does not change errno
  return failed;
}
