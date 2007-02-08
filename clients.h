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
  char *_name;
  char *_hostname;
  char *_ip_address;
  char *_protocol;
  char *_username;
  char *_password;
  char *_listfile;
  int mount_share(const char *mount_point, const char *path);
  int unmount_share(const char *mount_point);
public:
  Client(const char *name);
  ~Client();
  void setHostname(const char *value);
  void setIpAddress(const char *value);
  void setProtocol(const char *value);
  void setUsername(const char *value);
  void setPassword(const char *value);
  void setListfile(const char *value);
  int  backup(const char *mount_point, bool configcheck = false);
  void show();
};

class Clients : public vector<Client *> {
public:
  ~Clients() {
    for (unsigned int i = 0; i < size(); i++) {
      delete (*this)[i];
    }
  }
  int backup(
      const char *mount_point,
      bool       configcheck = false) {
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
