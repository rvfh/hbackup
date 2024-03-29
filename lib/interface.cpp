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
#include <fstream>
#include <list>
#include <errno.h>

using namespace std;

#include "strings.h"
#include "files.h"
#include "db.h"
#include "clients.h"
#include "hbackup.h"

using namespace hbackup;

struct HBackup::Private {
  const char    *default_db_path;
  Database*     db;
  list<String>  selected_clients;
  list<Client*> clients;
};

HBackup::HBackup() {
  _d                  = new Private;
  _d->default_db_path = "/backup";
  delete _d->db;
  _d->selected_clients.clear();
  _d->clients.clear();
}

HBackup::~HBackup() {
  for (list<Client*>::iterator client = _d->clients.begin();
      client != _d->clients.end(); client++){
    delete *client;
  }
  free(_d->db);
  _d->default_db_path = NULL;
  delete _d;
}

int HBackup::addClient(const char* name) {
  int cmp = 1;
  list<String>::iterator client = _d->selected_clients.begin();
  while ((client != _d->selected_clients.end())
      && ((cmp = client->compare(name)) < 0)) {
    client++;
  }
  if (cmp == 0) {
    cerr << "Error: client already selected: " << name << endl;
    return -1;
  }
  _d->selected_clients.insert(client, name);
  return 0;
}

int HBackup::readConfig(const char* config_path) {
  /* Open configuration file */
  ifstream config_file(config_path);

  if (! config_file.is_open()) {
    cerr << strerror(errno) << ": " << config_path << endl;
    return -1;
  } else {
    /* Read configuration file */
    string  buffer;
    int     line    = 0;

    if (verbosity() > 1) {
      cout << " -> Reading configuration file" << endl;
    }

    Client* client = NULL;
    while (! config_file.eof()) {
      getline(config_file, buffer);
      unsigned int pos = buffer.find("\r");
      if (pos != string::npos) {
        buffer.erase(pos);
      }
      list<string> params;

      line++;
      if (Stream::decodeLine(buffer, params)) {
        errno = EUCLEAN;
        cerr << "Warning: in file " << config_path << ", line " << line
          << " missing single- or double-quote" << endl;
      }
      if (params.size() == 1) {
        errno = EUCLEAN;
        cerr << "Error: in file " << config_path << ", line " << line
          << " unexpected lonely keyword: " << *params.begin() << endl;
        return -1;
      } else if (params.size() > 1) {
        list<string>::iterator current = params.begin();
        string                 keyword = *current++;

        if (keyword == "db") {
          if (params.size() > 2) {
            cerr << "Error: in file " << config_path << ", line " << line
              << " '" << keyword << "' takes exactly one argument" << endl;
            return -1;
          } else
          if (_d->db != NULL) {
            cerr << "Error: in file " << config_path << ", line " << line
              << " '" << keyword << "' seen twice" << endl;
            return -1;
          } else {
            _d->db = new Database(*current);
          }
        } else if (keyword == "client") {
          if (params.size() != 3) {
            cerr << "Error: in file " << config_path << ", line " << line
              << " '" << keyword << "' takes exactly two arguments" << endl;
            return -1;
          }
          string protocol = *current++;
          client = new Client(*current);
          client->setProtocol(protocol);

          int cmp = 1;
          list<Client*>::iterator i = _d->clients.begin();
          while ((i != _d->clients.end())
              && ((cmp = client->prefix().compare((*i)->prefix())) > 0)) {
            i++;
          }
          if (cmp == 0) {
            cerr << "Error: client already selected: " << *current << endl;
            return -1;
          }
          _d->clients.insert(i, client);
        } else if (client != NULL) {
          if (keyword == "hostname") {
            if (params.size() > 2) {
              cerr << "Error: in file " << config_path << ", line " << line
                << " '" << keyword << "' takes exactly one argument" << endl;
              return -1;
            }
            client->setHostOrIp(*current);
          } else
          if (keyword == "option") {
            if (params.size() > 3) {
              cerr << "Error: in file " << config_path << ", line " << line
                << " '" << keyword << "' takes one or two arguments" << endl;
              return -1;
            }
            if (params.size() == 2) {
              client->addOption(*current);
            } else {
              string name = *current++;
              client->addOption(name, *current);
            }
          } else
          if (keyword == "listfile") {
            if (params.size() > 2) {
              cerr << "Error: in file " << config_path << ", line " << line
                << " '" << keyword << "' takes exactly one argument" << endl;
              return -1;
            }
            client->setListfile(current->c_str());
          } else {
            cerr << "Unrecognised keyword '" << keyword
              << "' in configuration file, line " << line
              << endl;
            return -1;
          }
        } else {
          cerr << "Error: in file " << config_path << ", line " << line
            << " keyword before client" << endl;
          return -1;
        }
      }
    }
    config_file.close();
  }

  // Use default DB path if none specified
  if (_d->db == NULL) {
    _d->db = new Database(_d->default_db_path);
  }
  return 0;
}

int HBackup::check(bool thorough) {
  if (! _d->db->open()) {
    bool failed = false;

    if (_d->db->scan("", thorough)) {
      failed = true;
    }
    _d->db->close();
    if (! failed) {
      return 0;
    }
  }
  return -1;
}

int HBackup::backup(bool config_check) {
  if (! _d->db->open()) {
    bool failed = false;

    for (list<Client*>::iterator client = _d->clients.begin();
        client != _d->clients.end(); client++) {
      if (terminating()) {
        break;
      }
      // Skip unrequested clients
      if (_d->selected_clients.size() != 0) {
        bool found = false;
        for (list<String>::iterator i = _d->selected_clients.begin();
          i != _d->selected_clients.end(); i++) {
          if (*i == (*client)->name().c_str()) {
            found = true;
            break;
          }
        }
        if (! found) {
          continue;
        }
      }
      (*client)->setMountPoint(_d->db->path() + "/mount");
      if ((*client)->backup(*_d->db, config_check)) {
        failed = true;
      }
    }
    _d->db->close();
    if (failed) {
      return -1;
    }
  }
  return -1;
}
