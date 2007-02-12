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

class Option {
  string _name;
  string _value;
public:
  Option(const string& name, const string& value) : _name(name), _value(value) {}
  string name() { return _name; }
  string value() { return _value; }
  string option() {
    if (_name.size() != 0)
      return _name + "=" + _value;
    else
      return _value;
  }
};

class Client {
  string _name;
  vector<Option> _options;
  string _host_or_ip;
  string _protocol;
  string _listfile;
  int mount_share(const string& mount_point, const string& path);
  int unmount_share(const string& mount_point);
public:
  Client(string name);
  void addOption(const string& name, const string& value) {
    _options.push_back(Option(name, value));
  }
  void setHostOrIp(string value);
  void setProtocol(string value);
  void setListfile(string value);
  int  backup(string mount_point, bool configcheck = false);
  void show();
};

class Clients : public vector<Client *> {
public:
  ~Clients() {
    for (unsigned int i = 0; i < size(); i++) {
      delete (*this)[i];
    }
  }
  int backup(string mount_point, bool configcheck = false) {
    int failed = 0;
    for (unsigned int i = 0; i < size(); i++) {
      if ((*this)[i]->backup(mount_point, configcheck)) {
        failed = 1;
      }
    }
    return failed;
  }
};

#endif
