/* Herve Fache

20061009 Creation
*/

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "list.h"
#include "filters.h"
#include "filelist.h"
#include "cvs_parser.h"
/* Yes, include C file */
#include "db.c"

/* List of files */
list_t file_list;

static void file_data_show(const void *payload, char *string) {
  strcpy(string, ((metadata_t *) payload)->path);
}

static void db_data_show(const void *payload, char *string) {
  const db_data_t *db_data = payload;

  /* Times are omitted here... */
  sprintf(string, "'%s' '%s' %c %ld %ld %u %u 0%o '%s' %s %ld %ld %c",
    db_data->prefix, db_data->metadata.path,
    type_letter(db_data->metadata.type), db_data->metadata.size,
    db_data->metadata.mtime * 0, db_data->metadata.uid, db_data->metadata.gid,
    db_data->metadata.mode, db_data->link, db_data->checksum,
    db_data->date_in * 0, db_data->date_out * 0, '-');
}

int main(void) {
  void *handle = NULL;
  char checksum[40];
  int  status;

  /* Test internal functions */
  printf("type_letter\n");
  printf("File   : %c\n", type_letter(S_IFREG));
  printf("Dir    : %c\n", type_letter(S_IFDIR));
  printf("Char   : %c\n", type_letter(S_IFCHR));
  printf("Block  : %c\n", type_letter(S_IFBLK));
  printf("FIFO   : %c\n", type_letter(S_IFIFO));
  printf("Link   : %c\n", type_letter(S_IFLNK));
  printf("Socket : %c\n", type_letter(S_IFSOCK));
  printf("Unknown: %c\n", type_letter(0));

  printf("type_mode\n");
  printf("File   : 0%06o\n", type_mode('f'));
  printf("Dir    : 0%06o\n", type_mode('d'));
  printf("Char   : 0%06o\n", type_mode('c'));
  printf("Block  : 0%06o\n", type_mode('b'));
  printf("FIFO   : 0%06o\n", type_mode('p'));
  printf("Link   : 0%06o\n", type_mode('l'));
  printf("Socket : 0%06o\n", type_mode('s'));
  printf("Unknown: 0%06o\n", type_mode('?'));

  /* Use other modules */
  if ((status = parsers_new())) {
    printf("parsers_new error status %u\n", status);
    return 0;
  }
  if ((status = parsers_add(cvs_parser_new()))) {
    printf("parsers_add error status %u\n", status);
    return 0;
  }

  if ((status = filters_new(&handle))) {
    printf("parsers_new error status %u\n", status);
    return 0;
  }
  if ((status = filters_add(handle, "test/subdir", filter_path_start))) {
    printf("ignore_add error status %u\n", status);
    return 0;
  }
  if ((status = filters_add(handle, "~", filter_end))) {
    printf("ignore_add error status %u\n", status);
    return 0;
  }
  if ((status = filters_add(handle, "\\.o$", filter_file_regexp))) {
    printf("ignore_add error status %u\n", status);
    return 0;
  }

  if ((status = file_list_new("test////", handle))) {
    printf("file_list_new error status %u\n", status);
    return 0;
  }
  list_show(file_list_get(), NULL, file_data_show);

  /* Test database */
  if ((status = db_open("test_db"))) {
    printf("db_open error status %u\n", status);
    if (status == 2) {
      return 0;
    }
  }

  /* Write and read back */
  if ((status = db_write("test/testfile", checksum))) {
    printf("db_write error status %u\n", status);
    db_close();
    return 0;
  }
  printf("%s  test/testfile\n", checksum);
  if ((status = db_read("test_db/blah", checksum))) {
    printf("db_read error status %u\n", status);
    db_close();
    return 0;
  }

  if ((status = db_parse("file://host/share", "test", file_list_get()))) {
    printf("db_parse error status %u\n", status);
    db_close();
    return 0;
  }
  list_show(db_list, NULL, db_data_show);

  db_close();

  if ((status = db_open("test_db"))) {
    printf("db_open error status %u\n", status);
    if (status == 2) {
      return 0;
    }
  }
  list_show(db_list, NULL, db_data_show);
  db_close();

  file_list_free();
  filters_free(handle);
  parsers_free();

  return 0;
}
