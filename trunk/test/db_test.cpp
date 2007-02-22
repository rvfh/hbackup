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

using namespace std;

#include "db.cpp"
#include "filters.h"
#include "parsers.h"
#include "cvs_parser.h"
#include "paths.h"

static int verbose = 3;

static string db_data_show(const void *payload) {
  const db_data_t *db_data = (db_data_t *) payload;
  string d_in;
  string d_out;
  if (db_data->date_in == 0) d_in = "0"; else d_in = "1";
  if (db_data->date_out == 0) d_out = "0"; else d_out = "1";
  return db_data->filedata->line(true) + "\t" + d_in + "\t" + d_out;
}

static string parse_select(const void *payload) {
  const db_data_t *db_data = (const db_data_t *) (payload);

  if (db_data->date_out != 0) {
    /* This string cannot be matched */
    return "\t";
  } else {
    return db_data->filedata->path();
  }
}

int verbosity(void) {
  return verbose;
}

int terminating(void) {
  return 0;
}

int main(void) {
  Path*     path;
  string    checksum;
  string    zchecksum;
  db_data_t db_data;
  off_t     size;
  off_t     zsize;
  List      *filelist;
  int       status;

  /* Test internal functions */
  File::zcopy("test/testfile", "test_db/testfile.gz", &size, &zsize, &checksum,
    &zchecksum, 5);
  cout << "Copied " << size << " -> " << zsize << " bytes: "
    << checksum << " -> " << zchecksum << endl;

  File::zcopy("test_db/testfile.gz", "/dev/null", &size, &zsize, &checksum,
    &zchecksum, -1);
  cout << "Copied " << size << " -> " << zsize << " bytes: "
    << checksum << " -> " << zchecksum << endl;

  File::zcopy("test2/testfile~", "test_db/testfile.gz", &size, NULL, &checksum,
    NULL, 5);
  cout << "Copied " << size << " -> ? bytes " << checksum << " -> ?" << endl;

  File::zcopy("test2/testfile~", "test_db/testfile.gz", NULL, &zsize, NULL,
    &zchecksum, 9);
  cout << "Copied ? -> " << zsize << " bytes ? -> " << zchecksum << endl;

  /* Use other modules */
  path = new Path("");
  path->addParser("all", "cvs");
  path->addFilter("type", "dir");
  path->addFilter("path_start", ".svn", true);
  path->addFilter("type", "dir");
  path->addFilter("path_start", "subdir", true);
  path->addFilter("type", "file");
  path->addFilter("path_end", "~", true);
  path->addFilter("type", "file");
  path->addFilter("path_regexp", "\\.o$", true);
  if (path->createList("test////")) {
    cout << "file list is empty" << endl;
    return 0;
  }
  cout << ">List " << path->list()->size() << " file(s):" << endl;
  for (list<File>::iterator i = path->list()->begin();
    i != path->list()->end(); i++) {
    cout << (*i).line(true) << endl;
  }

  Database db("test_db");

  /* Test database */
  if ((status = db.open())) {
    printf("db_open error status %u\n", status);
    if (status == 2) {
      return 0;
    }
  }

  /* Write check */
  db_data.filedata = new File("test/testfile");
  db_data.date_in = time(NULL);
  db_data.date_out = 0;
  if ((status = db.write("test/", "testfile", &db_data, checksum, 0))) {
    printf("db.write error status %u\n", status);
    db.close();
    return 0;
  }
  cout << checksum << "  test/testfile" << endl;
  filelist = new List(db_data_show);
  db.load("data/59ca0efa9f5633cb0371bbc0355478d8-0/list", filelist);
  cout << ">List " << filelist->size() << " element(s):\n";
  filelist->show(NULL, db_data_show);
  delete filelist;

  /* Obsolete check */
  db_data.filedata->setChecksum(checksum);
  db.obsolete(*db_data.filedata);
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

  if ((status = db.parse("file://host", "/home/user", "test", path->list()))) {
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

  if ((status = db.parse("file://host", "/home/user", "test", path->list()))) {
    printf("db.parse error status %u\n", status);
    db.close();
    return 0;
  }
  cout << ">List " << db_list->size() << " element(s):\n";
  db_list->show(NULL, db_data_show);
  delete path;

  path = new Path("");
  path->addParser("all", "cvs");
  path->addFilter("type", "dir");
  path->addFilter("path_start", ".svn", true);
  path->addFilter("type", "dir");
  path->addFilter("path_start", "subdir", true);
  path->addFilter("type", "file");
  path->addFilter("path_end", "~", true);
  path->addFilter("type", "file");
  path->addFilter("path_regexp", "\\.o$", true);
  if (path->createList("test2")) {
    cout << "file list is empty" << endl;
    return 0;
  }
  cout << ">List " << path->list()->size() << " file(s):" << endl;
  for (list<File>::iterator i = path->list()->begin();
    i != path->list()->end(); i++) {
    cout << (*i).line(true) << endl;
  }

  if ((status = db.parse("file://host", "/home/user2", "test2",
      path->list()))) {
    printf("db.parse error status %u\n", status);
    db.close();
    return 0;
  }
  cout << ">List " << db_list->size() << " element(s):\n";
  db_list->show(NULL, db_data_show);
  delete path;

  filelist = new List(db_data_show);
  db.load("data/d41d8cd98f00b204e9800998ecf8427e-0/list", filelist);
  cout << ">List " << filelist->size() << " element(s):\n";
  filelist->show(NULL, db_data_show);
  delete filelist;

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
  File::zcopy("test_db/list", "test_db/list.save", NULL, NULL, NULL, NULL, 0);

  db.open();

  remove("test_db/data/59ca0efa9f5633cb0371bbc0355478d8-0/data");
  if ((status = db.scan())) {
    printf("full scan error status %u\n", status);
  }

  db.close();

  // Restore list
  File::zcopy("test_db/list.save", "test_db/list", NULL, NULL, NULL, NULL, 0);

  db.open();

  if ((status = db.scan("", true))) {
    printf("full thorough scan error status %u\n", status);
  }

  db.close();

  // Restore list
  File::zcopy("test_db/list.save", "test_db/list", NULL, NULL, NULL, NULL, 0);

  db.open();

  File::testDir("test_db/data/59ca0efa9f5633cb0371bbc0355478d8-0", 1);
  File::testReg("test_db/data/59ca0efa9f5633cb0371bbc0355478d8-0/data", 1);
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

  path = new Path("");
  path->addParser("all", "cvs");
  path->addFilter("type", "dir");
  path->addFilter("path_start", ".svn", true);
  path->addFilter("type", "dir");
  path->addFilter("path_start", "subdir", true);
  path->addFilter("type", "file");
  path->addFilter("path_end", "~", true);
  path->addFilter("type", "file");
  path->addFilter("path_regexp", "\\.o$", true);
  if (path->createList("test////")) {
    cout << "file list is empty" << endl;
    return 0;
  }
  cout << ">List " << path->list()->size() << " file(s):" << endl;
  for (list<File>::iterator i = path->list()->begin();
    i != path->list()->end(); i++) {
    cout << (*i).line(true) << endl;
  }

  if ((status = db.parse("file://host", "/home/user", "test", path->list()))) {
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

  path = new Path("");
  path->addParser("all", "cvs");
  path->addFilter("type", "dir");
  path->addFilter("path_start", ".svn", true);
  path->addFilter("type", "dir");
  path->addFilter("path_start", "subdir", true);
  path->addFilter("type", "file");
  path->addFilter("path_end", "~", true);
  path->addFilter("type", "file");
  path->addFilter("path_regexp", "\\.o$", true);
  if (path->createList("test////")) {
    cout << "file list is empty" << endl;
    return 0;
  }
  cout << ">List " << path->list()->size() << " file(s):" << endl;
  for (list<File>::iterator i = path->list()->begin();
    i != path->list()->end(); i++) {
    cout << (*i).line(true) << endl;
  }

  if ((status = db.parse("file://host", "/home/user", "test",
      path->list()))) {
    printf("db.parse error status %u\n", status);
    db.close();
    return 0;
  }
  cout << ">List " << db_list->size() << " element(s):\n";
  db_list->show(NULL, db_data_show);
  cout << ">List " << db_list->size() << " element(s):\n";
  db_list->show(NULL, parse_select);
  delete path;

  path = new Path("");
  path->addParser("all", "cvs");
  path->addFilter("type", "dir");
  path->addFilter("path_start", ".svn", true);
  path->addFilter("type", "dir");
  path->addFilter("path_start", "subdir", true);
  path->addFilter("type", "file");
  path->addFilter("path_end", "~", true);
  path->addFilter("type", "file");
  path->addFilter("path_regexp", "\\.o$", true);
  if (path->createList("test////")) {
    cout << "file list is empty" << endl;
    return 0;
  }
  cout << ">List " << path->list()->size() << " file(s):" << endl;
  for (list<File>::iterator i = path->list()->begin();
    i != path->list()->end(); i++) {
    cout << (*i).line(true) << endl;
  }

  if ((status = db.parse("file://host", "/home/user", "test",
      path->list()))) {
    printf("db.parse error status %u\n", status);
    db.close();
    return 0;
  }
  cout << ">List " << db_list->size() << " element(s):\n";
  db_list->show(NULL, db_data_show);
  delete path;

  path = new Path("");
  path->addParser("all", "cvs");
  path->addFilter("type", "dir");
  path->addFilter("path_start", ".svn", true);
  path->addFilter("type", "dir");
  path->addFilter("path_start", "subdir", true);
  path->addFilter("type", "file");
  path->addFilter("path_end", "~", true);
  path->addFilter("type", "file");
  path->addFilter("path_regexp", "\\.o$", true);
  if (path->createList("test2")) {
    cout << "file list is empty" << endl;
    return 0;
  }
  cout << ">List " << path->list()->size() << " file(s):" << endl;
  for (list<File>::iterator i = path->list()->begin();
    i != path->list()->end(); i++) {
    cout << (*i).line(true) << endl;
  }

  if ((status = db.parse("file://host", "/home/user2", "test2",
      path->list()))) {
    printf("db.parse error status %u\n", status);
    db.close();
    return 0;
  }
  cout << ">List " << db_list->size() << " element(s):\n";
  db_list->show(NULL, db_data_show);
  delete path;

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

  cout << endl << "Test: getdir" << endl;
  cout << "Check test_db/data dir: " << File::testDir("test_db/data", true) << endl;
  File::testReg("test_db/data/.nofiles", true);
  File::testDir("test_db/data/fe", true);
  File::testReg("test_db/data/fe/.nofiles", true);
  File::testReg("test_db/data/fe/test4", true);
  File::testDir("test_db/data/fe/dc", true);
  File::testReg("test_db/data/fe/dc/.nofiles", true);
  File::testDir("test_db/data/fe/ba", true);
  File::testDir("test_db/data/fe/ba/test1", true);
  File::testDir("test_db/data/fe/98", true);
  File::testDir("test_db/data/fe/98/test2", true);
  string  getdir_path;
  cout << "febatest1 status: " << db.getDir("febatest1", getdir_path, true)
    << ", getdir_path: " << getdir_path << endl;
  cout << "fe98test2 status: " << db.getDir("fe98test2", getdir_path, true)
    << ", getdir_path: " << getdir_path << endl;
  cout << "fe98test3 status: " << db.getDir("fe98test3", getdir_path, true)
    << ", getdir_path: " << getdir_path << endl;
  cout << "fetest4 status: " << db.getDir("fetest4", getdir_path, true)
    << ", getdir_path: " << getdir_path << endl;
  cout << "fedc76test5 status: " << db.getDir("fedc76test5", getdir_path, true)
    << ", getdir_path: " << getdir_path << endl;
  File::testDir("test_db/data/fe/dc/76", true);
  cout << "fedc76test6 status: " << db.getDir("fedc76test6", getdir_path, true)
    << ", getdir_path: " << getdir_path << endl;
  mkdir("test_db/data/fe/dc/76/test6", 0755);
  cout << "fedc76test6 status: " << db.getDir("fedc76test6", getdir_path, true)
    << ", getdir_path: " << getdir_path << endl;

  db.close();


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
