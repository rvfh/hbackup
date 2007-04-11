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

int verbosity(void) {
  return 3;
}

int terminating(void) {
  return 0;
}

int main(void) {
  Clients   *clients;
  Client    *client;
  Database  db("test_db");

  remove("test_db/list");

  clients = new Clients;
  clients->setMountPoint("test_db/mount");
  printf(">List %u client(s):\n", clients->size());
  for (unsigned int i = 0; i < clients->size(); i++) {
    (*clients)[i]->show();
  }
  client = new Client("localhost");
  clients->push_back(client);
  client->setProtocol("file");
  client->setHostOrIp("localhost");
  client->setListfile("etc/doesnotexist");
  printf(">List %u client(s):\n", clients->size());
  for (unsigned int i = 0; i < clients->size(); i++) {
    (*clients)[i]->show();
  }
  clients->backup(db, 0);
  delete clients;

  clients = new Clients;
  clients->setMountPoint("test_db/mount");
  printf(">List %u client(s):\n", clients->size());
  for (unsigned int i = 0; i < clients->size(); i++) {
    (*clients)[i]->show();
  }
  client = new Client("myClient");
  clients->push_back(client);
  client->setProtocol("nfs");
  client->setHostOrIp("myClient");
  client->setListfile("/home/User/hbackup.list");
  printf(">List %u client(s):\n", clients->size());
  for (unsigned int i = 0; i < clients->size(); i++) {
    (*clients)[i]->show();
  }
  client = new Client("myClient2");
  clients->push_back(client);
  client->setProtocol("smb");
  client->setHostOrIp("myClient");
  client->addOption("username", "Myself");
  client->addOption("password", "flesyM");
  client->setListfile("C:\\Backup\\Backup.LST");
  printf(">List %u client(s):\n", clients->size());
  for (unsigned int i = 0; i < clients->size(); i++) {
    (*clients)[i]->show();
  }
  client = new Client("otherClient");
  clients->push_back(client);
  client->setProtocol("ssh");
  client->setHostOrIp("otherClient");
  client->setListfile("c:/home/backup/Backup.list");
  printf(">List %u client(s):\n", clients->size());
  for (unsigned int i = 0; i < clients->size(); i++) {
    (*clients)[i]->show();
  }
  client = new Client("Client");
  clients->push_back(client);
  client->setProtocol("smb");
  client->setHostOrIp("Client");
  client->addOption("username", "user");
  client->addOption("iocharset", "utf8");
  client->setListfile("c:/home/BlaH/Backup.list");
  printf(">List %u client(s):\n", clients->size());
  for (unsigned int i = 0; i < clients->size(); i++) {
    (*clients)[i]->show();
  }
  client = new Client("Client2");
  clients->push_back(client);
  client->setProtocol("smb");
  client->setHostOrIp("Client");
  client->addOption("nocase");
  client->addOption("username", "user");
  client->addOption("password", "");
  client->setListfile("c:/home/BlaH/Backup.list");
  printf(">List %u client(s):\n", clients->size());
  for (unsigned int i = 0; i < clients->size(); i++) {
    (*clients)[i]->show();
  }
  client = new Client("Client3");
  clients->push_back(client);
  client->setProtocol("smb");
  client->setHostOrIp("Client");
  client->addOption("username", "");
  client->setListfile("c:/home/BlaH/Backup.list");
  printf(">List %u client(s):\n", clients->size());
  for (unsigned int i = 0; i < clients->size(); i++) {
    (*clients)[i]->show();
  }
  client = new Client("Client4");
  clients->push_back(client);
  client->setProtocol("smb");
  client->setHostOrIp("Client");
  client->addOption("username", "");
  client->addOption("password", "");
  client->setListfile("c:/home/BlaH/Backup.list");
  printf(">List %u client(s):\n", clients->size());
  for (unsigned int i = 0; i < clients->size(); i++) {
    (*clients)[i]->show();
  }
  db.open();
  clients->backup(db, 0);
  delete clients;
  db.close();

  clients = new Clients;
  clients->setMountPoint("test_db/mount");
  client = new Client("testhost");
  clients->push_back(client);
  client->setProtocol("file");
  client->setHostOrIp("localhost");
  client->setListfile("etc/localhost.list");
  printf(">List %u client(s):\n", clients->size());
  for (unsigned int i = 0; i < clients->size(); i++) {
    (*clients)[i]->show();
  }
  db.open();
  clients->backup(db, 0);
  delete clients;
  db.close();

  clients = new Clients;
  clients->setMountPoint("test_db/mount");
  client = new Client("testhost");
  clients->push_back(client);
  client->setProtocol("file");
  client->setListfile("etc/localhost.list");
  printf(">List %u client(s):\n", clients->size());
  for (unsigned int i = 0; i < clients->size(); i++) {
    (*clients)[i]->show();
  }
  db.open();
  clients->backup(db, 0);
  delete clients;
  db.close();

  return 0;
}
