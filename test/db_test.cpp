/*
     Copyright (C) 2006  Herve Fache

     This program is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License version 2 as
     published by the Free Software Foundation.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place - Suite 330,
     Boston, MA 02111-1307, USA.
*/

#include "db.cpp"
#include "parsers.h"
#include "cvs_parser.h"

static int verbose = 3;

static char *file_data_show(const void *payload) {
  char *string = NULL;

  asprintf(&string, "%s", ((filedata_t *) payload)->path.c_str());
  return string;
}

static char *db_data_show(const void *payload) {
  const db_data_t *db_data = (db_data_t *) payload;
  char *string = NULL;

  /* Times are omitted here... */
  asprintf(&string, "'%s' '%s' %c %ld %d %u %u 0%o '%s' %s %d %d %c",
    db_data->host.c_str(), db_data->filedata.path.c_str(),
    type_letter(db_data->filedata.metadata.type),
    db_data->filedata.metadata.size,
    db_data->filedata.metadata.mtime || 0, db_data->filedata.metadata.uid,
    db_data->filedata.metadata.gid, db_data->filedata.metadata.mode,
    db_data->link.c_str(), db_data->filedata.checksum, db_data->date_in || 0,
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
  Filter    *filters = NULL;
  Parsers   *parsers = NULL;
  char      checksum[40];
  char      zchecksum[40];
  db_data_t db_data;
  off_t     size;
  off_t     zsize;
  List      *filelist;
  int       status;

  /* Test internal functions */
  zcopy("test/testfile", "test_db/testfile.gz", &size, &zsize, checksum,
    zchecksum, 5);
  printf("Copied %ld -> %ld bytes: %s -> %s\n",
    size, zsize, checksum, zchecksum);

  zcopy("test_db/testfile.gz", "/dev/null", &size, &zsize, checksum,
    zchecksum, -1);
  printf("Copied %ld -> %ld bytes: %s -> %s\n",
    size, zsize, checksum, zchecksum);

  zcopy("test2/testfile~", "test_db/testfile.gz", &size, NULL, checksum,
    NULL, 5);
  printf("Copied %ld -> ? bytes %s -> ?\n", size, checksum);

  zcopy("test2/testfile~", "test_db/testfile.gz", NULL, &zsize, NULL,
    zchecksum, 9);
  printf("Copied ? -> %ld bytes ? -> %s\n", zsize, zchecksum);

  /* Use other modules */
  parsers = new Parsers;
  parsers->push_back(new CvsParser(parser_controlled));

  filters = new Filter;
  filters->push_back(new Rule(new Condition(S_IFDIR, filter_path_start, ".svn")));
  filters->push_back(new Rule(new Condition(S_IFDIR, filter_path_start, "subdir")));
  filters->push_back(new Rule(new Condition(S_IFREG, filter_path_end, "~")));
  filters->push_back(new Rule(new Condition(S_IFREG, filter_path_regexp, "\\.o$")));

  FileList *file_list;
  file_list = new FileList("test////", filters, parsers);
  if (file_list->getList() == NULL) {
    cout << "file_list is NULL" << endl;
    return 0;
  }
  cout << ">List " << file_list->getList()->size() << " file(s):" << endl;
  file_list->getList()->show(NULL, file_data_show);

  Database db("test_db");

  /* Test database */
  if ((status = db.open())) {
    printf("db_open error status %u\n", status);
    if (status == 2) {
      return 0;
    }
  }

  /* Write check */
  db_data.filedata.path = "test/testfile";
  metadata_get(db_data.filedata.path.c_str(), &db_data.filedata.metadata);
  db_data.host = "this is a host";
  db_data.link = "this is a link";
  db_data.date_in = time(NULL);
  db_data.date_out = 0;
  if ((status = db.write("test/", "testfile", &db_data, checksum, 0))) {
    printf("db.write error status %u\n", status);
    db.close();
    return 0;
  }
  printf("%s  test/testfile\n", checksum);
  filelist = new List(db_data_show);
  db.load("data/59ca0efa9f5633cb0371bbc0355478d8-0/list", filelist);
  cout << ">List " << filelist->size() << " element(s):\n";
  filelist->show(NULL, db_data_show);
  delete filelist;

  /* Obsolete check */
  db.obsolete(db_data.host, db_data.filedata.path, checksum);
  filelist = new List(db_data_show);
  db.load("data/59ca0efa9f5633cb0371bbc0355478d8-0/list", filelist);
  cout << ">List " << filelist->size() << " element(s):\n";
  filelist->show(NULL, db_data_show);
  delete filelist;

  /* Read check */
  if ((status = db.read("test_db/blah", checksum))) {
    printf("db.read error status %u\n", status);
    db.close();
    return 0;
  }

  if ((status = db.parse("file://host", "/home/user", "test",
      file_list->getList()))) {
    printf("db.parse error status %u\n", status);
    db.close();
    return 0;
  }
  cout << ">List " << db_list->size() << " element(s):\n";
  db_list->show(NULL, db_data_show);

  db.close();

  /* Re-open database => no change */
  if ((status = db.open())) {
    printf("db_open error status %u\n", status);
    if (status == 2) {
      return 0;
    }
  }
  cout << ">List " << db_list->size() << " element(s):\n";
  db_list->show(NULL, db_data_show);

  if ((status = db.parse("file://host", "/home/user", "test",
      file_list->getList()))) {
    printf("db.parse error status %u\n", status);
    db.close();
    return 0;
  }
  cout << ">List " << db_list->size() << " element(s):\n";
  db_list->show(NULL, db_data_show);
  delete file_list;

  file_list = new FileList("test2", filters, parsers);
  if (file_list->getList() == NULL) {
    cout << "file_list_new error status " << status << endl;
    return 0;
  }
  cout << ">List " << file_list->getList()->size() << " file(s):" << endl;
  file_list->getList()->show(NULL, file_data_show);

  if ((status = db.parse("file://host", "/home/user2", "test2",
      file_list->getList()))) {
    printf("db.parse error status %u\n", status);
    db.close();
    return 0;
  }
  cout << ">List " << db_list->size() << " element(s):\n";
  db_list->show(NULL, db_data_show);
  delete file_list;

  verbose = 2;
  if ((status = db.scan("59ca0efa9f5633cb0371bbc0355478d8-0"))) {
    printf("scan error status %u\n", status);
    if (status) {
      return 0;
    }
  }

  if ((status = db.scan("59ca0efa9f5633cb0371bbc0355478d8-0", true))) {
    printf("scan error status %u\n", status);
    if (status) {
      return 0;
    }
  }

  db.close();
  db.open();


  if ((status = db.scan())) {
    printf("full scan error status %u\n", status);
    if (status) {
      return 0;
    }
  }

  db.close();
  db.open();

  if ((status = db.scan("", true))) {
    printf("full thorough scan error status %u\n", status);
    if (status) {
      return 0;
    }
  }

  db.close();

  // Save list
  zcopy("test_db/list", "test_db/list.save", NULL, NULL, NULL, NULL, 0);

  db.open();

  remove("test_db/data/59ca0efa9f5633cb0371bbc0355478d8-0/data");
  if ((status = db.scan())) {
    printf("full scan error status %u\n", status);
  }

  db.close();

  // Restore list
  zcopy("test_db/list.save", "test_db/list", NULL, NULL, NULL, NULL, 0);

  db.open();

  if ((status = db.scan("", true))) {
    printf("full thorough scan error status %u\n", status);
  }

  db.close();

  // Restore list
  zcopy("test_db/list.save", "test_db/list", NULL, NULL, NULL, NULL, 0);

  db.open();

  testdir("test_db/data/59ca0efa9f5633cb0371bbc0355478d8-0", 1);
  testfile("test_db/data/59ca0efa9f5633cb0371bbc0355478d8-0/data", 1);
  if ((status = db.scan("", true))) {
    printf("full thorough scan error status %u\n", status);
  }
  verbose = 3;

  db.close();



  /* Re-open database => correct missing file */
  if ((status = db.open())) {
    printf("db_open error status %u\n", status);
    if (status == 2) {
      return 0;
    }
  }
  cout << ">List " << db_list->size() << " element(s):\n";
  db_list->show(NULL, db_data_show);

  file_list = new FileList("test////", filters, parsers);
  if (file_list->getList() == NULL) {
    cout << "file_list_new error status " << status << endl;
    return 0;
  }
  cout << ">List " << file_list->getList()->size() << " file(s):" << endl;
  file_list->getList()->show(NULL, file_data_show);

  if ((status = db.parse("file://host", "/home/user", "test",
      file_list->getList()))) {
    printf("db.parse error status %u\n", status);
    db.close();
    return 0;
  }
  cout << ">List " << db_list->size() << " element(s):\n";
  db_list->show(NULL, db_data_show);

  db.close();



  /* Re-open database => remove some files */
  if ((status = db.open())) {
    printf("db_open error status %u\n", status);
    if (status == 2) {
      return 0;
    }
  }
  cout << ">List " << db_list->size() << " element(s):\n";
  db_list->show(NULL, db_data_show);

  remove("test/testfile");

  file_list = new FileList("test////", filters, parsers);
  if (file_list->getList() == NULL) {
    cout << "file_list_new error status " << status << endl;
    return 0;
  }
  cout << ">List " << file_list->getList()->size() << " file(s):" << endl;
  file_list->getList()->show(NULL, file_data_show);

  if ((status = db.parse("file://host", "/home/user", "test",
      file_list->getList()))) {
    printf("db.parse error status %u\n", status);
    db.close();
    return 0;
  }
  cout << ">List " << db_list->size() << " element(s):\n";
  db_list->show(NULL, db_data_show);
  cout << ">List " << db_list->size() << " element(s):\n";
  db_list->show(NULL, parse_select);
  delete file_list;

  file_list = new FileList("test////", filters, parsers);
  if (file_list->getList() == NULL) {
    cout << "file_list_new error status " << status << endl;
    return 0;
  }
  cout << ">List " << file_list->getList()->size() << " file(s):" << endl;
  file_list->getList()->show(NULL, file_data_show);

  if ((status = db.parse("file://host", "/home/user", "test",
      file_list->getList()))) {
    printf("db.parse error status %u\n", status);
    db.close();
    return 0;
  }
  cout << ">List " << db_list->size() << " element(s):\n";
  db_list->show(NULL, db_data_show);
  delete file_list;

  file_list = new FileList("test2", filters, parsers);
  if (file_list->getList() == NULL) {
    cout << "file_list_new error status " << status << endl;
    return 0;
  }
  cout << ">List " << file_list->getList()->size() << " file(s):" << endl;
  file_list->getList()->show(NULL, file_data_show);

  if ((status = db.parse("file://host", "/home/user2", "test2",
      file_list->getList()))) {
    printf("db.parse error status %u\n", status);
    db.close();
    return 0;
  }
  cout << ">List " << db_list->size() << " element(s):\n";
  db_list->show(NULL, db_data_show);
  delete file_list;

  db.organize("test_db/data", 2);

  db.close();


  /* Re-open database => one less active item */
  if ((status = db.open())) {
    printf("db_open error status %u\n", status);
    if (status == 2) {
      return 0;
    }
  }
  cout << ">List " << db_list->size() << " element(s):\n";
  db_list->show(NULL, db_data_show);

  db.close();


  delete filters;
  delete parsers;


  /* List cannot be saved */
  if ((status = db.open())) {
    printf("db_open error status %u\n", status);
    if (status == 2) {
      return 0;
    }
  }
  system("chmod ugo-w test_db");
  db.close();
  printf("Error: %s\n", strerror(errno));
  system("chmod u+w test_db");
  system("rm -f test_db/lock");

  /* List cannot be read */
  system("chmod ugo-r test_db/list");
  if ((status = db.open())) {
    printf("Error: %s\n", strerror(errno));
  }

  /* List is garbaged */
  system("chmod u+r test_db/list");
  system("echo blah >> test_db/list");
  if ((status = db.open())) {
    printf("Error: %s\n", strerror(errno));
  }

  /* List is gone */
  remove("test_db/list");
  if ((status = db.open())) {
    printf("Error: %s\n", strerror(errno));
  }

  return 0;
}
