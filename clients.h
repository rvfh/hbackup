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

class Client {
  string _name;
  string _host_or_ip;
  string _protocol;
  string _username;
  string _password;
  string _listfile;
  int mount_share(string mount_point, string path);
  int unmount_share(string mount_point);
public:
  Client(string name);
  void setHostOrIp(string value);
  void setProtocol(string value);
  void setUsername(string value);
  void setPassword(string value);
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
