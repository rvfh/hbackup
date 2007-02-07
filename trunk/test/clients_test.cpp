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

#include <iostream>
#include <vector>
#include <errno.h>
#include "metadata.h"
#include "common.h"
#include "list.h"
#include "filters.h"
#include "db.h"
#include "clients.h"

int verbosity(void) {
  return 3;
}

int terminating(void) {
  return 0;
}

int main(void) {
  Clients *clients;
  Client  *client;

  remove("test_db/list");

  clients = new Clients;
  printf(">List %u client(s):\n", clients->size());
  for (unsigned int i = 0; i < clients->size(); i++) {
    (*clients)[i]->show();
  }
  client = new Client("file://localhost", "etc/doesnotexist");
  clients->push_back(client);
  printf(">List %u client(s):\n", clients->size());
  for (unsigned int i = 0; i < clients->size(); i++) {
    (*clients)[i]->show();
  }
  clients->backup("test_db/mount", 0);
  delete clients;

  clients = new Clients;
  printf(">List %u client(s):\n", clients->size());
  for (unsigned int i = 0; i < clients->size(); i++) {
    (*clients)[i]->show();
  }
  client = new Client("nFs://myClient", "/home/User/devel");
  clients->push_back(client);
  printf(">List %u client(s):\n", clients->size());
  for (unsigned int i = 0; i < clients->size(); i++) {
    (*clients)[i]->show();
  }
  client = new Client("Smb://Myself:flesyM@myClient", "C:\\Backup\\Backup.LST");
  clients->push_back(client);
  printf(">List %u client(s):\n", clients->size());
  for (unsigned int i = 0; i < clients->size(); i++) {
    (*clients)[i]->show();
  }
  client = new Client("sSh://otherClient", "c:/home/backup/Backup.list");
  clients->push_back(client);
  printf(">List %u client(s):\n", clients->size());
  for (unsigned int i = 0; i < clients->size(); i++) {
    (*clients)[i]->show();
  }
  client = new Client("Smb://user@Client", "c:/home/BlaH/Backup.list");
  clients->push_back(client);
  printf(">List %u client(s):\n", clients->size());
  for (unsigned int i = 0; i < clients->size(); i++) {
    (*clients)[i]->show();
  }
  client = new Client("Smb://user:@Client", "c:/home/BlaH/Backup.list");
  clients->push_back(client);
  printf(">List %u client(s):\n", clients->size());
  for (unsigned int i = 0; i < clients->size(); i++) {
    (*clients)[i]->show();
  }
  client = new Client("Smb://@Client", "c:/home/BlaH/Backup.list");
  clients->push_back(client);
  printf(">List %u client(s):\n", clients->size());
  for (unsigned int i = 0; i < clients->size(); i++) {
    (*clients)[i]->show();
  }
  client = new Client("Smb://:@Client", "c:/home/BlaH/Backup.list");
  clients->push_back(client);
  printf(">List %u client(s):\n", clients->size());
  for (unsigned int i = 0; i < clients->size(); i++) {
    (*clients)[i]->show();
  }
  db_open("test_db");
  clients->backup("test_db/mount", 0);
  delete clients;
  db_close();

  clients = new Clients;
  client = new Client("file://localhost", "etc/localhost.list");
  clients->push_back(client);
  db_open("test_db");
  clients->backup("test_db/mount", 0);
  delete clients;
  db_close();

  clients = new Clients;
  client = new Client("file://localhost", "etc/localhost.list");
  clients->push_back(client);
  db_open("test_db");
  clients->backup("test_db/mount", 0);
  delete clients;
  db_close();

  return 0;
}
