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
  strcpy(string, ((filedata_t *) payload)->path);
}

static void db_data_show(const void *payload, char *string) {
  const db_data_t *db_data = payload;

  /* Times are omitted here... */
  sprintf(string, "'%s' '%s' %c %ld %ld %u %u 0%o '%s' %s %ld %ld %c",
    db_data->prefix, db_data->filedata.path,
    type_letter(db_data->filedata.metadata.type), db_data->filedata.metadata.size,
    db_data->filedata.metadata.mtime * 0, db_data->filedata.metadata.uid,
    db_data->filedata.metadata.gid, db_data->filedata.metadata.mode, db_data->link,
    db_data->filedata.checksum, db_data->date_in * 0, db_data->date_out * 0, '-');
}

int main(void) {
  void *filters_handle = NULL;
  void *parsers_handle = NULL;
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
  if ((status = parsers_new(&parsers_handle))) {
    printf("parsers_new error status %u\n", status);
    return 0;
  }
  if ((status = parsers_add(parsers_handle, cvs_parser_new()))) {
    printf("parsers_add error status %u\n", status);
    return 0;
  }

  if ((status = filters_new(&filters_handle))) {
    printf("parsers_new error status %u\n", status);
    return 0;
  }
  if ((status = filters_add(filters_handle, ".svn", filter_file_start))) {
    printf("ignore_add error status %u\n", status);
    return 0;
  }
  if ((status = filters_add(filters_handle, "subdir", filter_path_start))) {
    printf("ignore_add error status %u\n", status);
    return 0;
  }
  if ((status = filters_add(filters_handle, "~", filter_end))) {
    printf("ignore_add error status %u\n", status);
    return 0;
  }
  if ((status = filters_add(filters_handle, "\\.o$", filter_file_regexp))) {
    printf("ignore_add error status %u\n", status);
    return 0;
  }

  if ((status = filelist_new("test////", filters_handle, parsers_handle))) {
    printf("file_list_new error status %u\n", status);
    return 0;
  }
  list_show(filelist_getlist(), NULL, file_data_show);

  /* Test database */
  if ((status = db_open("test_db"))) {
    printf("db_open error status %u\n", status);
    if (status == 2) {
      return 0;
    }
  }

  /* Write and read back */
  if ((status = db_write("test/", "testfile", 13, checksum))) {
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

  if ((status = db_parse("file://host/share", filelist_getpath(),
      filelist_getlist()))) {
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

  if ((status = db_scan("d41d8cd98f00b204e9800998ecf8427e-0"))) {
    printf("db_scan error status %u\n", status);
    if (status == 2) {
      return 0;
    }
  }

/*  if ((status = db_check("d41d8cd98f00b204e9800998ecf8427e-0"))) {
    printf("db_check error status %u\n", status);
    if (status == 2) {
      return 0;
    }
  }
*/
  db_close();

  filelist_free();
  filters_free(filters_handle);
  parsers_free(parsers_handle);

  return 0;
}
