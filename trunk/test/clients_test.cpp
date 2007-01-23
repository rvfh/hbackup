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

#include "clients.cpp"

static int verbose = 3;

static char *client_show(const void *payload) {
  const client_t *client  = (client_t *) payload;
  char  *credentials      = NULL;
  char  *string           = NULL;

  if (client->username != NULL) {
    char *password = NULL;

    if (client->password != NULL) {
      asprintf(&password, ":%s", client->password);
    } else {
      asprintf(&password, "%s", "");
    }
    asprintf(&credentials, "%s%s@", client->username, password);
    free(password);
  } else {
    asprintf(&credentials, "%s", "");
  }
  asprintf(&string, "%s://%s%s %s", client->protocol, credentials,
    client->hostname, client->listfile);
  free(credentials);
  return string;
}

int verbosity(void) {
  return verbose;
}

int terminating(void) {
  return 0;
}

int main(void) {
  remove("test_db/list");
  if (clients_new()) {
    printf("Failed to create\n");
    return 1;
  }
  printf(">List %u client(s):\n", clients->size());
  clients->show(NULL, client_show);
  clients_add("file://localhost", "etc/doesnotexist");
  printf(">List %u client(s):\n", clients->size());
  clients->show(NULL, client_show);
  clients_backup("test_db/mount", 0);
  clients_free();

  if (clients_new()) {
    printf("Failed to create\n");
    return 1;
  }
  printf(">List %u client(s):\n", clients->size());
  clients->show(NULL, client_show);
  clients_add("nFs://myClient", "/home/User/devel");
  printf(">List %u client(s):\n", clients->size());
  clients->show(NULL, client_show);
  clients_add("Smb://Myself:flesyM@myClient", "C:\\Backup\\Backup.LST");
  printf(">List %u client(s):\n", clients->size());
  clients->show(NULL, client_show);
  clients_add("sSh://otherClient", "c:/home/backup/Backup.list");
  printf(">List %u client(s):\n", clients->size());
  clients->show(NULL, client_show);
  clients_add("Smb://user@Client", "c:/home/BlaH/Backup.list");
  printf(">List %u client(s):\n", clients->size());
  clients->show(NULL, client_show);
  clients_add("Smb://user:@Client", "c:/home/BlaH/Backup.list");
  printf(">List %u client(s):\n", clients->size());
  clients->show(NULL, client_show);
  clients_add("Smb://@Client", "c:/home/BlaH/Backup.list");
  printf(">List %u client(s):\n", clients->size());
  clients->show(NULL, client_show);
  clients_add("Smb://:@Client", "c:/home/BlaH/Backup.list");
  printf(">List %u client(s):\n", clients->size());
  clients->show(NULL, client_show);
  db_open("test_db");
  clients_backup("test_db/mount", 0);
  db_close();
  clients_free();

  if (clients_new()) {
    printf("Failed to create\n");
    return 1;
  }
  clients_add("file://localhost", "etc/localhost.list");
  db_open("test_db");
  verbose = 2;
  clients_backup("test_db/mount", 0);
  verbose = 3;
  db_close();
  clients_free();

  if (clients_new()) {
    printf("Failed to create\n");
    return 1;
  }
  clients_add("file://localhost", "etc/localhost.list");
  db_open("test_db");
  verbose = 2;
  clients_backup("test_db/mount", 0);
  verbose = 3;
  db_close();
  clients_free();

  return 0;
}
