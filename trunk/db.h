/* Herve Fache

20061008 Creation
*/

#ifndef DB_H
#define DB_H

/* Open database */
extern int db_open(const char *db_path);

/* Close database */
extern void db_close(void);

/* Check what needs to be done for given prefix (protocol://host/share) */
extern int db_parse(const char *prefix, const char *real_path,
  const char *mount_path, list_t list);

/* Read file with given checksum, extract it to path */
extern int db_read(const char *path, const char *checksum);

/* Check that data for given checksum exists (if NULL, scan all) */
extern int db_scan(const char *checksum);

/* Check that data for given checksum is not corrupted (if NULL, check all) */
extern int db_check(const char *checksum);

#endif