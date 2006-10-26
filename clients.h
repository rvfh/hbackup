/* Herve Fache

20061023 Creation
*/

#ifndef CLIENT_H
#define CLIENT_H

#define _GNU_SOURCE
#include <stdio.h>

/* Create clients management */
extern int clients_new(void);

/* Add client data */
extern void clients_free(void);

/* Backup all clients */
extern int clients_backup(const char *mount_point);

/* Destroy clients management */
extern int clients_add(const char *info, const char *listfile);

#endif
