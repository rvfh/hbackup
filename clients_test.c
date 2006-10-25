/* Herve Fache

20061023 Creation
*/

#include "clients.c"

static void client_show(const void *payload, char *string) {
  const client_t *client_t = payload;

  sprintf(string, "%s://", client_t->protocol);
  if (client_t->username != NULL) {
    strcat(string, client_t->username);
    if (client_t->password != NULL) {
      strcat(string, ":");
      strcat(string, client_t->password);
    }
    strcat(string, "@");
  }
  strcat(string, client_t->hostname);
  strcat(string, " ");
  strcat(string, client_t->listfile);
}

int verbosity(void) {
  return 2;
}

int main(void) {
  remove("test_db/list");
  if (clients_new()) {
    printf("Failed to create\n");
    return 1;
  }
  list_show(clients, NULL, client_show);
  clients_add("file://localhost", "etc/doesnotexist");
  list_show(clients, NULL, client_show);
  clients_backup();
  clients_free();

  if (clients_new()) {
    printf("Failed to create\n");
    return 1;
  }
  list_show(clients, NULL, client_show);
  clients_add("file://localhost", "etc/localhost.list");
  list_show(clients, NULL, client_show);
  clients_add("Smb://Myself:flesyM@myClient", "C:\\Backup\\Backup.LST");
  list_show(clients, NULL, client_show);
  clients_add("sSh://otherClient", "/home/backup/Backup.list");
  list_show(clients, NULL, client_show);
  db_open("test_db");
  clients_backup();
  db_close();
  clients_free();

  if (clients_new()) {
    printf("Failed to create\n");
    return 1;
  }
  clients_add("file://localhost", "etc/localhost.list");
  db_open("test_db");
  clients_backup();
  db_close();
  clients_free();

  return 0;
}
