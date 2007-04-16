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

#ifndef CLIENT_H
#define CLIENT_H

#ifndef PATHS_H
#error You must include paths.h before clients.h
#endif

#ifndef HBACKUP_H
#error You must include hbackup.h before clients.h
#endif

class Option {
  string _name;
  string _value;
public:
  Option(const string& name, const string& value) :
    _name(name), _value(value) {}
  string name() { return _name; }
  string value() { return _value; }
  string option() {
    if (_name.empty())
      return _value;
    else
      return _name + "=" + _value;
  }
};

class Client {
  string          _name;
  string          _host_or_ip;
  string          _listfilename;
  string          _listfiledir;
  string          _protocol;
  vector<Option*> _options;
  vector<Path*>   _paths;
  string          _mount_point;
  string          _mounted;
  int mountPath(
    string        backup_path,
    string        *path);
  int umount();
  int readListFile(const string& list_path);
public:
  Client(string name);
  ~Client();
  void addOption(const string& value) {
    _options.push_back(new Option("", value));
  }
  void addOption(const string& name, const string& value) {
    _options.push_back(new Option(name, value));
  }
  void setHostOrIp(string value);
  void setProtocol(string value);
  void setListfile(string value);
  int  setMountPoint(const string& mount_point, bool check = true) {
    _mount_point = mount_point;
    /* Check that mount dir exists, if not create it */
    if (check && (File::testDir(_mount_point, true) == 2)) {
      cerr << "Cannot create mount point" << endl;
      return 2;
    }
    return 0;
  }
  string mountPoint() { return _mount_point; }
  int  backup(Database& db, bool config_check = false);
  void show();
};

class Clients : public vector<Client *> {
  string _mount_point;
public:
  Clients() : _mount_point("") {}
  ~Clients() {
    for (unsigned int i = 0; i < size(); i++) {
      delete (*this)[i];
    }
  }
  int setMountPoint(const string& mount_point) {
    _mount_point = mount_point;
    /* Check that mount dir exists, if not create it */
    if (File::testDir(_mount_point, true) == 2) {
      cerr << "Cannot create mount point" << endl;
      return 2;
    }
    return 0;
  }
  string mountPoint() { return _mount_point; }
  int backup(Database& db, bool config_check = false) {
    int failed = 0;
    for (unsigned int i = 0; i < size(); i++) {
      if (terminating()) {
        break;
      }
      if ((_mount_point.size() != 0)
       && ((*this)[i]->mountPoint().size() == 0)) {
        (*this)[i]->setMountPoint(_mount_point, false);
      }
      if ((*this)[i]->backup(db, config_check)) {
        failed = 1;
      }
    }
    return failed;
  }
};

#endif