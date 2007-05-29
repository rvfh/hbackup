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

#ifndef CLIENT_H
#define CLIENT_H

#ifndef PATHS_H
#error You must include paths.h before clients.h
#endif

#ifndef HBACKUP_H
#error You must include hbackup.h before clients.h
#endif

namespace hbackup {

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
  string        _name;
  string        _host_or_ip;
  string        _listfilename;
  string        _listfiledir;
  string        _protocol;
  list<Option>  _options;
  //
  bool          _initialised;
  list<Path>    _paths;
  string        _mount_point;
  string        _mounted;
  int mountPath(string  backup_path, string  *path);
  int umount();
  int readListFile(const string& list_path);
public:
  Client(string name);
  string name() { return _name; }
  void addOption(const string& value) {
    _options.push_back(Option("", value));
  }
  void addOption(const string& name, const string& value) {
    _options.push_back(Option(name, value));
  }
  void setHostOrIp(string value);
  void setProtocol(string value);
  void setListfile(string value);
  //
  bool initialised() { return _initialised; }
  void setInitialised() { _initialised = true; }
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
  int  expire(Database& db);
  void show();
};

}

#endif