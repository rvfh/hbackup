/* Herve Fache

20061023 Creation
*/

#include "clients.c"

static void client_show(const void *payload, char **string_p) {
  const client_t *client_t     = payload;
  char           *credentials  = NULL;
  /* No to warn about null string */
  char           null_string[] = "";

  if (client_t->username != NULL) {
    char *password = NULL;

    if (client_t->password != NULL) {
      asprintf(&password, ":%s", client_t->password);
    } else {
      asprintf(&password, null_string);
    }
    asprintf(&credentials, "%s%s@", client_t->username, password);
    free(password);
  } else {
    asprintf(&credentials, null_string);
  }
  asprintf(string_p, "%s://%s%s %s", client_t->protocol, credentials,
    client_t->hostname, client_t->listfile);
  free(credentials);
}

int verbosity(void) {
  return 2;
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
  list_show(clients, NULL, client_show);
  clients_add("file://localhost", "etc/doesnotexist");
  list_show(clients, NULL, client_show);
  clients_backup("test_db/mount");
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
  clients_backup("test_db/mount");
  db_close();
  clients_free();

  if (clients_new()) {
    printf("Failed to create\n");
    return 1;
  }
  clients_add("file://localhost", "etc/localhost.list");
  db_open("test_db");
  clients_backup("test_db/mount");
  db_close();
  clients_free();

  return 0;
}
