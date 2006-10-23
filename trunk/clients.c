/* Herve Fache

20061023 Creation
*/

#include <stdlib.h>
#include <string.h>
#include "list.h"
#include "params.h"
#include "clients.h"

static list_t clients = NULL;

int clients_new(void) {
  /* Create new list */
  clients = list_new(NULL);
  if (clients == NULL) {
    fprintf(stderr, "client: new: cannot intialise\n");
    return 2;
  }
  return 0;
}

void clients_free(void) {
  list_free(clients);
}

int clients_add(const char *info, const char *listfile) {
  client_t *client = malloc(sizeof(client_t));
  const char *start = info;
  char *delim = strchr(start, ':');

  if (delim == NULL) {
    return 1;
  }
  /* Protocol */
  strncpy(client->protocol, start, delim - start);
  /* Lower case */
  strtolower(client->protocol);
  start = delim + 1;
  /* There should be two / now */
  if (*start != '/') {
    return 1;
  }
  start++;
  if (*start != '/') {
    return 1;
  }
  start++;
  /* Host name */
  delim = strchr(start, '@');
  if (delim == NULL) {
    /* No username/password */
    strcpy(client->username, "");
    strcpy(client->password, "");
    strcpy(client->hostname, start);
    /* Lower case */
    strtolower(client->hostname);
  } else {
    char *colon = strchr(start, ':');

    strcpy(client->hostname, delim + 1);
    /* Lower case */
    strtolower(client->hostname);
    if (colon == NULL) {
      /* No password */
      strncpy(client->username, start, delim - start);
      strcpy(client->password, "");
    } else {
      strncpy(client->username, start, colon - start);
      strncpy(client->password, colon + 1, delim - colon - 1);
    }
  }

  /* List file */
  strcpy(client->listfile, listfile);
  if (! strcmp(client->protocol, "smb")) {
    /* Lower case */
    strtolower(client->listfile);
    pathtolinux(client->listfile);
  }

  list_append(clients, client);
  return 0;
}
