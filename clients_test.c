/* Herve Fache

20061023 Creation
*/

#include "clients.c"

static int verbose = 3;

static char *client_show(const void *payload) {
  const client_t *client = payload;
  char  *credentials       = NULL;
  char  *string            = NULL;
  /* No to get a warning about null string */
  char  null_string[]      = "";

  if (client->username != NULL) {
    char *password = NULL;

    if (client->password != NULL) {
      asprintf(&password, ":%s", client->password);
    } else {
      asprintf(&password, "%s", null_string);
    }
    asprintf(&credentials, "%s%s@", client->username, password);
    free(password);
  } else {
    asprintf(&credentials, "%s", null_string);
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
  clients_add("nFs://myClient", "/home/User/devel");
  list_show(clients, NULL, client_show);
  clients_add("Smb://Myself:flesyM@myClient", "C:\\Backup\\Backup.LST");
  list_show(clients, NULL, client_show);
  clients_add("sSh://otherClient", "c:/home/backup/Backup.list");
  list_show(clients, NULL, client_show);
  clients_add("Smb://user@Client", "c:/home/BlaH/Backup.list");
  list_show(clients, NULL, client_show);
  clients_add("Smb://user:@Client", "c:/home/BlaH/Backup.list");
  list_show(clients, NULL, client_show);
  clients_add("Smb://@Client", "c:/home/BlaH/Backup.list");
  list_show(clients, NULL, client_show);
  clients_add("Smb://:@Client", "c:/home/BlaH/Backup.list");
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
  verbose = 2;
  clients_backup("test_db/mount");
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
  clients_backup("test_db/mount");
  verbose = 3;
  db_close();
  clients_free();

  return 0;
}
