/* Herve Fache

20061009 Creation
*/

#include "filters.h"
#include "cvs_parser.h"
/* Yes, include C file */
#include "db.c"

static int verbose = 3;

static char *file_data_show(const void *payload) {
  char *string = NULL;

  asprintf(&string, "%s", ((filedata_t *) payload)->path);
  return string;
}

static char *db_data_show(const void *payload) {
  const db_data_t *db_data = payload;
  char  *link = "";
  char *string = NULL;

  if (db_data->link != NULL) {
    link = db_data->link;
  }
  /* Times are omitted here... */
  asprintf(&string, "'%s' '%s' %c %ld %d %u %u 0%o '%s' %s %d %d %c",
    db_data->host, db_data->filedata.path,
    type_letter(db_data->filedata.metadata.type),
    db_data->filedata.metadata.size,
    db_data->filedata.metadata.mtime || 0, db_data->filedata.metadata.uid,
    db_data->filedata.metadata.gid, db_data->filedata.metadata.mode, link,
    db_data->filedata.checksum, db_data->date_in || 0,
    db_data->date_out || 0, '-');
  return string;
}

int verbosity(void) {
  return verbose;
}

int terminating(void) {
  return 0;
}

int main(void) {
  list_t    compress_handle = NULL;
  list_t    filters_handle = NULL;
  list_t    parsers_handle = NULL;
  char      checksum[40];
  char      zchecksum[40];
  db_data_t db_data;
  size_t    size;
  size_t    zsize;
  list_t    filelist;
  int       status;

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

  zcopy("test/testfile", "test_db/testfile.gz", &size, &zsize, checksum,
    zchecksum, 5);
  printf("Copied %u -> %u bytes: %s -> %s\n",
    size, zsize, checksum, zchecksum);

  zcopy("test_db/testfile.gz", "test/testfile", &size, &zsize, checksum,
    zchecksum, -1);
  printf("Copied %u -> %u bytes: %s -> %s\n",
    size, zsize, checksum, zchecksum);

  zcopy("test2/testfile~", "test_db/testfile.gz", &size, NULL, checksum,
    NULL, 5);
  printf("Copied %u -> ? bytes %s -> ?\n", size, checksum);

  zcopy("test2/testfile~", "test_db/testfile.gz", NULL, &zsize, NULL,
    zchecksum, 9);
  printf("Copied ? -> %u bytes ? -> %s\n", zsize, zchecksum);

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
    printf("ignore_new error status %u\n", status);
    return 0;
  }
  if ((status = filters_rule_add(filters_rule_new(filters_handle), S_IFDIR,
      filter_path_start, ".svn"))) {
    printf("ignore_add error status %u\n", status);
    return 0;
  }
  if ((status = filters_rule_add(filters_rule_new(filters_handle), S_IFDIR,
      filter_path_start, "subdir"))) {
    printf("ignore_add error status %u\n", status);
    return 0;
  }
  if ((status = filters_rule_add(filters_rule_new(filters_handle), S_IFREG,
      filter_path_end, "~"))) {
    printf("ignore_add error status %u\n", status);
    return 0;
  }
  if ((status = filters_rule_add(filters_rule_new(filters_handle), S_IFREG,
      filter_path_regexp, "\\.o$"))) {
    printf("ignore_add error status %u\n", status);
    return 0;
  }
  if ((status = filters_new(&compress_handle))) {
    printf("compress_new error status %u\n", status);
    return 0;
  }
  if ((status = filters_rule_add(filters_rule_new(compress_handle), S_IFREG,
      filter_path_end, "testfile"))) {
    printf("compress_add error status %u\n", status);
    return 0;
  }

  verbose = 2;
  if ((status = filelist_new("test////", filters_handle, parsers_handle))) {
    printf("file_list_new error status %u\n", status);
    return 0;
  }
  verbose = 3;
  list_show(filelist_get(), NULL, file_data_show);

  /* Test database */
  if ((status = db_open("test_db"))) {
    printf("db_open error status %u\n", status);
    if (status == 2) {
      return 0;
    }
  }

  /* Write check */
  db_data.filedata.path = "test/testfile";
  metadata_get(db_data.filedata.path, &db_data.filedata.metadata);
  db_data.host = "this is a host";
  db_data.link = "this is a link";
  db_data.date_in = time(NULL);
  db_data.date_out = 0;
  if ((status = db_write("test/", "testfile", &db_data, checksum, 0))) {
    printf("db_write error status %u\n", status);
    db_close();
    return 0;
  }
  printf("%s  test/testfile\n", checksum);
  filelist = list_new(db_data_show);
  db_load("data/59ca0efa9f5633cb0371bbc0355478d8-0/list", filelist);
  list_show(filelist, NULL, db_data_show);
  list_free(filelist);

  /* Obsolete check */
  db_obsolete(db_data.host, db_data.filedata.path, checksum);
  filelist = list_new(db_data_show);
  db_load("data/59ca0efa9f5633cb0371bbc0355478d8-0/list", filelist);
  list_show(filelist, NULL, db_data_show);
  list_free(filelist);

  /* Read check */
  if ((status = db_read("test_db/blah", checksum))) {
    printf("db_read error status %u\n", status);
    db_close();
    return 0;
  }

  if ((status = db_parse("file://host", "/home/user", "test",
      filelist_get(), compress_handle))) {
    printf("db_parse error status %u\n", status);
    db_close();
    return 0;
  }
  list_show(db_list, NULL, db_data_show);

  db_close();


  /* Re-open database => no change */
  if ((status = db_open("test_db"))) {
    printf("db_open error status %u\n", status);
    if (status == 2) {
      return 0;
    }
  }
  list_show(db_list, NULL, db_data_show);

  if ((status = db_parse("file://host", "/home/user", "test",
      filelist_get(), compress_handle))) {
    printf("db_parse error status %u\n", status);
    db_close();
    return 0;
  }
  list_show(db_list, NULL, db_data_show);
  filelist_free();

  verbose = 2;
  if ((status = filelist_new("test2", filters_handle, parsers_handle))) {
    printf("file_list_new error status %u\n", status);
    return 0;
  }
  verbose = 3;
  list_show(filelist_get(), NULL, file_data_show);

  if ((status = db_parse("file://host", "/home/user2", "test2",
      filelist_get(), compress_handle))) {
    printf("db_parse error status %u\n", status);
    db_close();
    return 0;
  }
  list_show(db_list, NULL, db_data_show);
  filelist_free();

  if ((status = db_scan("59ca0efa9f5633cb0371bbc0355478d8-0"))) {
    printf("db_scan error status %u\n", status);
    if (status) {
      return 0;
    }
  }

  if ((status = db_scan(NULL))) {
    printf("db_scan (full) error status %u\n", status);
    if (status) {
      return 0;
    }
  }

  if ((status = db_check("59ca0efa9f5633cb0371bbc0355478d8-0"))) {
    printf("db_check error status %u\n", status);
    if (status) {
      return 0;
    }
  }

  if ((status = db_check(NULL))) {
    printf("db_check (full) error status %u\n", status);
    if (status) {
      return 0;
    }
  }

  db_close();



  /* Re-open database => remove some files */
  if ((status = db_open("test_db"))) {
    printf("db_open error status %u\n", status);
    if (status == 2) {
      return 0;
    }
  }
  list_show(db_list, NULL, db_data_show);

  remove("test/testfile");

  verbose = 2;
  if ((status = filelist_new("test////", filters_handle, parsers_handle))) {
    printf("file_list_new error status %u\n", status);
    return 0;
  }
  verbose = 3;
  list_show(filelist_get(), NULL, file_data_show);

  if ((status = db_parse("file://host", "/home/user", "test",
      filelist_get(), compress_handle))) {
    printf("db_parse error status %u\n", status);
    db_close();
    return 0;
  }
  list_show(db_list, NULL, db_data_show);
  list_show(db_list, NULL, parse_select);
  filelist_free();

  verbose = 2;
  if ((status = filelist_new("test////", filters_handle, parsers_handle))) {
    printf("file_list_new error status %u\n", status);
    return 0;
  }
  verbose = 3;
  list_show(filelist_get(), NULL, file_data_show);

  if ((status = db_parse("file://host", "/home/user", "test",
      filelist_get(), compress_handle))) {
    printf("db_parse error status %u\n", status);
    db_close();
    return 0;
  }
  list_show(db_list, NULL, db_data_show);
  filelist_free();

  verbose = 2;
  if ((status = filelist_new("test2", filters_handle, parsers_handle))) {
    printf("file_list_new error status %u\n", status);
    return 0;
  }
  verbose = 3;
  list_show(filelist_get(), NULL, file_data_show);

  if ((status = db_parse("file://host", "/home/user2", "test2",
      filelist_get(), compress_handle))) {
    printf("db_parse error status %u\n", status);
    db_close();
    return 0;
  }
  list_show(db_list, NULL, db_data_show);
  filelist_free();

  db_organize("test_db/data", 2);

  db_close();


  filters_free(filters_handle);
  parsers_free(parsers_handle);

  return 0;
}
