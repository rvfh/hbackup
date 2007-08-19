/*
     Copyright (C) 2006-2007  Herve Fache

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

#include <iostream>
#include <string>
#include <list>
#include <sys/stat.h>
#include <errno.h>

#include "files.h"
#include "filters.h"
#include "parsers.h"
#include "cvs_parser.h"
#include "dbdata.h"
#include "dblist.h"
#include "db.h"
#include "paths.h"

using namespace hbackup;

#warning most tests gone in r259, were based on old path behaviour...

static int verbose = 4;

int verbosity(void) {
  return verbose;
}

int terminating(void) {
  return 0;
}

time_t time(time_t *t) {
  static time_t my_time = 0;
  return ++my_time;
}

int main(void) {
  string            checksum;
  string            zchecksum;
  DbList::iterator  i;
  DbList            journal;
  int               status;

  Database db("test_db");

  /* Test database */
  if ((status = db.open())) {
    printf("db_open error status %u\n", status);
    if (status == 2) {
      return 0;
    }
  }

  cout << endl << "Test: getdir" << endl;
  cout << "Check test_db/data dir: " << ! Directory("test_db/data").isValid() << endl;
  File("").create("test_db/data/.nofiles");
  Directory("").create("test_db/data/fe");
  File("").create("test_db/data/fe/.nofiles");
  File("").create("test_db/data/fe/test4");
  Directory("").create("test_db/data/fe/dc");
  File("").create("test_db/data/fe/dc/.nofiles");
  Directory("").create("test_db/data/fe/ba");
  Directory("").create("test_db/data/fe/ba/test1");
  Directory("").create("test_db/data/fe/98");
  Directory("").create("test_db/data/fe/98/test2");
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
  Directory("").create("test_db/data/fe/dc/76");
  cout << "fedc76test6 status: " << db.getDir("fedc76test6", getdir_path, true)
    << ", getdir_path: " << getdir_path << endl;
  mkdir("test_db/data/fe/dc/76/test6", 0755);
  cout << "fedc76test6 status: " << db.getDir("fedc76test6", getdir_path, true)
    << ", getdir_path: " << getdir_path << endl;

  cout << endl << "Test: write and read back" << endl;
  /* Write */
  char* chksm = NULL;
  if ((status = db.write("test1/testfile", &chksm))) {
    printf("db.write error status %u\n", status);
    db.close();
    return 0;
  }
  if (chksm == NULL) {
    printf("db.write returned unexpected null checksum\n");
    db.close();
    return 0;
  }
  cout << chksm << "  test1/testfile" << endl;
  /* Read */
  if ((status = db.read("test_db/blah", chksm))) {
    printf("db.read error status %u\n", status);
    db.close();
    return 0;
  }
  /* Write again */
  chksm = NULL;
  if ((status = db.write("test_db/blah", &chksm))) {
    printf("db.write error status %u\n", status);
    db.close();
    return 0;
  }
  if (chksm == NULL) {
    printf("db.write returned unexpected null checksum\n");
    db.close();
    return 0;
  }
  cout << chksm << "  test_db/blah" << endl;

  cout << endl << "Test: organise" << endl;
  mkdir("test_db/data/zz", 0755);
  mkdir("test_db/data/zz/000001", 0755);
  cout << "Lesser:" << endl;
  db.organise("test_db/data/zz", 2);
  system("find test_db/data/zz");
  mkdir("test_db/data/zz/000002", 0755);
  cout << "Equal:" << endl;
  db.organise("test_db/data/zz", 2);
  system("find test_db/data/zz");
  mkdir("test_db/data/zz/00/0003", 0755);
  cout << "Greater:" << endl;
  db.organise("test_db/data/zz/00", 2);
  system("find test_db/data/zz");


  cout << "Active list: " << ((DbList*)db.active())->size() << " element(s):\n";
  for (i = ((DbList*)db.active())->begin(); i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }
  cout << "Removed list: " << ((DbList*)db.removed())->size() << " element(s):\n";
  for (i = ((DbList*)db.removed())->begin(); i != ((DbList*)db.removed())->end(); i++) {
    i->line();
  }

  db.close();


  /* Re-open database => no change */
  if ((status = db.open())) {
    printf("db_open error status %u\n", status);
    if (status == 2) {
      return 0;
    }
  }
  cout << "Active list: " << ((DbList*)db.active())->size() << " element(s):\n";
  for (i = ((DbList*)db.active())->begin(); i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }
  cout << "Removed list: " << ((DbList*)db.removed())->size() << " element(s):\n";
  for (i = ((DbList*)db.removed())->begin(); i != ((DbList*)db.removed())->end(); i++) {
    i->line();
  }

  cout << "List select test" << endl;
  list<Node*> select_list;
  db.getList("prefix", "base_path", "rel_path", select_list);
  cout << "Got " << select_list.size() << " elements" << endl;
  if (select_list.size() != 0) {
    for (list<Node*>::iterator i = select_list.begin(); i != select_list.end();
        i++) {
      cout << "Name: " << (*i)->name() << endl;
      delete *i;
    }
  }
  db.getList("file://host", "/home/user", "cvs", select_list);
  cout << "Got " << select_list.size() << " elements" << endl;
  if (select_list.size() != 0) {
    for (list<Node*>::iterator i = select_list.begin(); i != select_list.end();
        i++) {
      cout << "Name: " << (*i)->name() << endl;
      delete *i;
    }
  }

  cout << "Active list: " << ((DbList*)db.active())->size() << " element(s):\n";
  for (i = ((DbList*)db.active())->begin(); i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }
  cout << "Removed list: " << ((DbList*)db.removed())->size() << " element(s):\n";
  for (i = ((DbList*)db.removed())->begin(); i != ((DbList*)db.removed())->end(); i++) {
    i->line();
  }

  verbose = 3;
  if ((status = db.scan("59ca0efa9f5633cb0371bbc0355478d8-0"))) {
    printf("scan error status %u\n", status);
    if (status) {
      return 0;
    }
  }
  verbose = 4;

  verbose = 3;
  if ((status = db.scan("59ca0efa9f5633cb0371bbc0355478d8-0", true))) {
    printf("scan error status %u\n", status);
    if (status) {
      return 0;
    }
  }
  verbose = 4;

  db.close();


  db.open();

  verbose = 3;
  if ((status = db.scan())) {
    printf("full scan error status %u\n", status);
    if (status) {
      return 0;
    }
  }
  verbose = 4;

  db.close();


  db.open();

  verbose = 3;
  if ((status = db.scan("", true))) {
    printf("full thorough scan error status %u\n", status);
    if (status) {
      return 0;
    }
  }
  verbose = 4;

  db.close();


  // Save list
  system("cp test_db/active test_db/active.save");

  db.open();

  remove("test_db/data/59ca0efa9f5633cb0371bbc0355478d8-0/data");
  verbose = 3;
  if ((status = db.scan())) {
    printf("full scan error status %u\n", status);
  }
  verbose = 4;

  db.close();


  // Restore list
  system("cp test_db/active.save test_db/active");

  db.open();

  verbose = 3;
  if ((status = db.scan("", true))) {
    printf("full thorough scan error status %u\n", status);
  }
  verbose = 4;

  db.close();


  // Restore list
  system("cp test_db/active.save test_db/active");

  db.open();

  Directory("").create("test_db/data/59ca0efa9f5633cb0371bbc0355478d8-0");
  File("").create("test_db/data/59ca0efa9f5633cb0371bbc0355478d8-0/data");
  verbose = 3;
  if ((status = db.scan("", true))) {
    printf("full thorough scan error status %u\n", status);
  }
  verbose = 4;

  db.close();


  /* Re-open database */
  if ((status = db.open())) {
    printf("db_open error status %u\n", status);
    if (status == 2) {
      return 0;
    }
  }
  cout << "Active list: " << ((DbList*)db.active())->size() << " element(s):\n";
  for (i = ((DbList*)db.active())->begin(); i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }
  cout << "Removed list: " << ((DbList*)db.removed())->size() << " element(s):\n";
  for (i = ((DbList*)db.removed())->begin(); i != ((DbList*)db.removed())->end(); i++) {
    i->line();
  }

  system("chmod 0775 test1/testdir");
  system("chmod 0775 test1/cvs/dirutd/CVS/Entries");

  cout << "Active list: " << ((DbList*)db.active())->size() << " element(s):\n";
  for (i = ((DbList*)db.active())->begin(); i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }
  cout << "Removed list: " << ((DbList*)db.removed())->size() << " element(s):\n";
  for (i = ((DbList*)db.removed())->begin(); i != ((DbList*)db.removed())->end(); i++) {
    i->line();
  }

  remove("test1/dir space/file space");
  verbose = 3;

  cout << "Active list: " << ((DbList*)db.active())->size() << " element(s):\n";
  for (i = ((DbList*)db.active())->begin(); i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }
  cout << "Removed list: " << ((DbList*)db.removed())->size() << " element(s):\n";
  for (i = ((DbList*)db.removed())->begin(); i != ((DbList*)db.removed())->end(); i++) {
    i->line();
  }

  db.close();


  /* Re-open database */
  if ((status = db.open())) {
    printf("db_open error status %u\n", status);
    if (status == 2) {
      return 0;
    }
  }
  cout << "Active list: " << ((DbList*)db.active())->size() << " element(s):\n";
  for (i = ((DbList*)db.active())->begin(); i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }
  cout << "Removed list: " << ((DbList*)db.removed())->size() << " element(s):\n";
  for (i = ((DbList*)db.removed())->begin(); i != ((DbList*)db.removed())->end(); i++) {
    i->line();
  }

  system("chmod 0777 test1/testdir");
  system("chmod 0777 test1/cvs/dirutd/CVS/Entries");

  cout << "Active list: " << ((DbList*)db.active())->size() << " element(s):\n";
  for (i = ((DbList*)db.active())->begin(); i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }
  cout << "Removed list: " << ((DbList*)db.removed())->size() << " element(s):\n";
  for (i = ((DbList*)db.removed())->begin(); i != ((DbList*)db.removed())->end(); i++) {
    i->line();
  }

  remove("test1/dir space/file space");
  verbose = 3;

  cout << "Active list: " << ((DbList*)db.active())->size() << " element(s):\n";
  for (i = ((DbList*)db.active())->begin(); i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }
  cout << "Removed list: " << ((DbList*)db.removed())->size() << " element(s):\n";
  for (i = ((DbList*)db.removed())->begin(); i != ((DbList*)db.removed())->end(); i++) {
    i->line();
  }

  db.close();


  /* Re-open database => remove some files */
  if ((status = db.open())) {
    printf("db_open error status %u\n", status);
    if (status == 2) {
      return 0;
    }
  }
  cout << "Active list: " << ((DbList*)db.active())->size() << " element(s):\n";
  for (i = ((DbList*)db.active())->begin(); i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }
  cout << "Removed list: " << ((DbList*)db.removed())->size() << " element(s):\n";
  for (i = ((DbList*)db.removed())->begin(); i != ((DbList*)db.removed())->end(); i++) {
    i->line();
  }

  remove("test1/testfile");
  remove("test1/cvs/dirutd/fileutd");

  cout << "Active list: " << ((DbList*)db.active())->size() << " element(s):\n";
  for (i = ((DbList*)db.active())->begin(); i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }
  cout << "Removed list: " << ((DbList*)db.removed())->size() << " element(s):\n";
  for (i = ((DbList*)db.removed())->begin(); i != ((DbList*)db.removed())->end(); i++) {
    i->line();
  }

  db.organise("test_db/data", 2);

  db.close();


  /* Re-open database => one less active item */
  if ((status = db.open())) {
    printf("db_open error status %u\n", status);
    if (status == 2) {
      return 0;
    }
  }
  cout << "Active list: " << ((DbList*)db.active())->size() << " element(s):\n";
  for (i = ((DbList*)db.active())->begin(); i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }
  cout << "Removed list: " << ((DbList*)db.removed())->size() << " element(s):\n";
  for (i = ((DbList*)db.removed())->begin(); i != ((DbList*)db.removed())->end(); i++) {
    i->line();
  }

  db.close();


  cout << "List cannot be saved" << endl;
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

  cout << "List cannot be read" << endl;
  system("chmod ugo-r test_db/active");
  if ((status = db.open())) {
    printf("Error: %s\n", strerror(errno));
  }

  cout << "List is garbaged" << endl;
  system("chmod u+r test_db/active");
  system("head test_db/active > blah && mv blah test_db/active");
  system("echo blah >> test_db/active");
  if ((status = db.open())) {
    printf("Error: %s\n", strerror(errno));
  } else {
    db.close();
  }

  cout << "List is gone" << endl;
  rename("test_db/active", "test_db/active.save");
  remove("test_db/active~");
  if ((status = db.open())) {
    printf("Error: %s\n", strerror(errno));
  }

  return 0;
}
