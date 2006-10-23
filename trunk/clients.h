/* Herve Fache

20061023 Creation
*/

#ifndef CLIENT_H
#define CLIENT_H

#define _GNU_SOURCE
#include <stdio.h>

typedef struct {
  char protocol[256];
  char username[256];
  char password[256];
  char hostname[256];
  char listfile[FILENAME_MAX];
} client_t;

/* Create clients management */
extern int clients_new(void);

/* Add client data */
extern void clients_free(void);

/* Backup all clients */
extern int clients_backup(void);

/* Destroy clients management */
extern int clients_add(const char *info, const char *listfile);

#endif
