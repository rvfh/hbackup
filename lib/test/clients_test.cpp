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

using namespace std;

#include <iostream>
#include <vector>
#include <list>
#include <errno.h>

#include "hbackup.h"
#include "files.h"
#include "list.h"
#include "filters.h"
#include "parsers.h"
#include "paths.h"
#include "db.h"
#include "clients.h"

using namespace hbackup;

int verbosity(void) {
  return 3;
}

int terminating(void) {
  return 0;
}

int main(void) {
  vector<Client> clients;
  vector<Client>::iterator client = clients.end();
  Database  db("test_db");

  remove("test_db/list");

  printf(">List %u client(s):\n", clients.size());
  for (vector<Client>::iterator i = clients.begin(); i != clients.end(); i++) {
    i->show();
  }
  client = clients.insert(clients.end(), Client("localhost"));
  client->setProtocol("file");
  client->setHostOrIp("localhost");
  client->setListfile("etc/doesnotexist");
  printf(">List %u client(s):\n", clients.size());
  for (vector<Client>::iterator i = clients.begin(); i != clients.end(); i++) {
    i->show();
  }
  for (vector<Client>::iterator i = clients.begin(); i != clients.end(); i++) {
    i->setMountPoint("test_db/mount") || i->backup(db, 0);
  }
  clients.clear();

  printf(">List %u client(s):\n", clients.size());
  for (vector<Client>::iterator i = clients.begin(); i != clients.end(); i++) {
    i->show();
  }
  client = clients.insert(clients.end(), Client("myClient"));
  client->setProtocol("nfs");
  client->setHostOrIp("myClient");
  client->setListfile("/home/User/hbackup.list");
  printf(">List %u client(s):\n", clients.size());
  for (vector<Client>::iterator i = clients.begin(); i != clients.end(); i++) {
    i->show();
  }
  client = clients.insert(clients.end(), Client("myClient2"));
  client->setProtocol("smb");
  client->setHostOrIp("myClient");
  client->addOption("username", "Myself");
  client->addOption("password", "flesyM");
  client->setListfile("C:\\Backup\\Backup.LST");
  printf(">List %u client(s):\n", clients.size());
  for (vector<Client>::iterator i = clients.begin(); i != clients.end(); i++) {
    i->show();
  }
  client = clients.insert(clients.end(), Client("otherClient"));
  client->setProtocol("ssh");
  client->setHostOrIp("otherClient");
  client->setListfile("c:/home/backup/Backup.list");
  printf(">List %u client(s):\n", clients.size());
  for (vector<Client>::iterator i = clients.begin(); i != clients.end(); i++) {
    i->show();
  }
  client = clients.insert(clients.end(), Client("Client"));
  client->setProtocol("smb");
  client->setHostOrIp("Client");
  client->addOption("username", "user");
  client->addOption("iocharset", "utf8");
  client->setListfile("c:/home/BlaH/Backup.list");
  printf(">List %u client(s):\n", clients.size());
  for (vector<Client>::iterator i = clients.begin(); i != clients.end(); i++) {
    i->show();
  }
  client = clients.insert(clients.end(), Client("Client2"));
  client->setProtocol("smb");
  client->setHostOrIp("Client");
  client->addOption("nocase");
  client->addOption("username", "user");
  client->addOption("password", "");
  client->setListfile("c:/home/BlaH/Backup.list");
  printf(">List %u client(s):\n", clients.size());
  for (vector<Client>::iterator i = clients.begin(); i != clients.end(); i++) {
    i->show();
  }
  client = clients.insert(clients.end(), Client("Client3"));
  client->setProtocol("smb");
  client->setHostOrIp("Client");
  client->addOption("username", "");
  client->setListfile("c:/home/BlaH/Backup.list");
  printf(">List %u client(s):\n", clients.size());
  for (vector<Client>::iterator i = clients.begin(); i != clients.end(); i++) {
    i->show();
  }
  client = clients.insert(clients.end(), Client("Client4"));
  client->setProtocol("smb");
  client->setHostOrIp("Client");
  client->addOption("username", "");
  client->addOption("password", "");
  client->setListfile("c:/home/BlaH/Backup.list");
  printf(">List %u client(s):\n", clients.size());
  for (vector<Client>::iterator i = clients.begin(); i != clients.end(); i++) {
    i->show();
  }
  db.open();
  for (vector<Client>::iterator i = clients.begin(); i != clients.end(); i++) {
    i->setMountPoint("test_db/mount") || i->backup(db, 0);
  }
  clients.clear();
  db.close();

  client = clients.insert(clients.end(), Client("testhost"));
  client->setProtocol("file");
  client->setHostOrIp("localhost");
  client->setListfile("etc/localhost.list");
  printf(">List %u client(s):\n", clients.size());
  for (vector<Client>::iterator i = clients.begin(); i != clients.end(); i++) {
    i->show();
  }
  db.open();
  for (vector<Client>::iterator i = clients.begin(); i != clients.end(); i++) {
    i->setMountPoint("test_db/mount") || i->backup(db, 0);
  }
  clients.clear();
  db.close();

  client = clients.insert(clients.end(), Client("testhost"));
  client->setProtocol("file");
  client->setListfile("etc/localhost.list");
  printf(">List %u client(s):\n", clients.size());
  for (vector<Client>::iterator i = clients.begin(); i != clients.end(); i++) {
    i->show();
  }
  db.open();
  for (vector<Client>::iterator i = clients.begin(); i != clients.end(); i++) {
    i->setMountPoint("test_db/mount") || i->backup(db, 0);
  }
  clients.clear();
  db.close();

  return 0;
}
