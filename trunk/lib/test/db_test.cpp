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
#include <vector>
#include <list>
#include <sys/stat.h>
#include <errno.h>

#include "list.h"
#include "files.h"
#include "filters.h"
#include "parsers.h"
#include "cvs_parser.h"
#include "paths.h"
#include "list.h"
#include "dbdata.h"
#include "dblist.h"
#include "db.h"

using namespace hbackup;

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
  Path*             path;
  string            checksum;
  string            zchecksum;
  DbData*           db_data;
  DbList::iterator  i;
  DbList            journal;
  int               status;

  /* Use other modules */
  path = new Path("");
  path->addParser("c", "cvs");
  path->addFilter("type", "dir");
  path->addFilter("path_start", ".svn", true);
  path->addFilter("type", "dir");
  path->addFilter("path_start", "subdir", true);
  path->addFilter("type", "file");
  path->addFilter("path_end", "~", true);
  path->addFilter("type", "file");
  path->addFilter("path_regex", "\\.o$", true);
  verbose = 3;
  if (path->createList("test1////")) {
    cout << "file list is empty" << endl;
    return 0;
  }
  verbose = 4;
  cout << ">List " << path->list()->size() << " file(s):" << endl;
  for (list<File>::iterator i = path->list()->begin();
    i != path->list()->end(); i++) {
    cout << i->line(true) << endl;
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
  db_data = new DbData(File("test1/testfile"));
  if ((status = db.write("test1/testfile", *db_data))) {
    printf("db.write error status %u\n", status);
    db.close();
    return 0;
  }
  cout << db_data->checksum() << "  test1/testfile" << endl;

  /* Read check */
  if ((status = db.read("test_db/blah", db_data->checksum()))) {
    printf("db.read error status %u\n", status);
    db.close();
    return 0;
  }

  if ((status = db.parse("file://host", "/home/user", "test1", path->list()))){
    printf("db.parse error status %u\n", status);
    db.close();
    return 0;
  }
  journal.load("test_db", "seen.journal");
  cout << "Added journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  journal.load("test_db", "gone.journal");
  cout << "Removed journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  journal.load("test_db", "written.journal");
  cout << "Really added journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  cout << "Active list: " << db.active()->size() << " element(s):\n";
  for (i = db.active()->begin(); i != db.active()->end(); i++) {
    cout << i->line(true) << endl;
  }
  cout << "Removed list: " << db.removed()->size() << " element(s):\n";
  for (i = db.removed()->begin(); i != db.removed()->end(); i++) {
    cout << i->line(true) << endl;
  }

  db.close();

  /* Re-open database => no change */
  if ((status = db.open())) {
    printf("db_open error status %u\n", status);
    if (status == 2) {
      return 0;
    }
  }
  cout << "Active list: " << db.active()->size() << " element(s):\n";
  for (i = db.active()->begin(); i != db.active()->end(); i++) {
    cout << i->line(true) << endl;
  }
  cout << "Removed list: " << db.removed()->size() << " element(s):\n";
  for (i = db.removed()->begin(); i != db.removed()->end(); i++) {
    cout << i->line(true) << endl;
  }

  if ((status = db.parse("file://host", "/home/user", "test1", path->list()))){
    printf("db.parse error status %u\n", status);
    db.close();
    return 0;
  }
  journal.load("test_db", "seen.journal");
  cout << "Added journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  journal.load("test_db", "gone.journal");
  cout << "Removed journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  journal.load("test_db", "written.journal");
  cout << "Really added journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  cout << "Active list: " << db.active()->size() << " element(s):\n";
  for (i = db.active()->begin(); i != db.active()->end(); i++) {
    cout << i->line(true) << endl;
  }
  cout << "Removed list: " << db.removed()->size() << " element(s):\n";
  for (i = db.removed()->begin(); i != db.removed()->end(); i++) {
    cout << i->line(true) << endl;
  }
  delete path;

  path = new Path("");
  path->addParser("c", "cvs");
  path->addFilter("type", "dir");
  path->addFilter("path_start", ".svn", true);
  path->addFilter("type", "dir");
  path->addFilter("path_start", "subdir", true);
  path->addFilter("type", "file");
  path->addFilter("path_end", "~", true);
  path->addFilter("type", "file");
  path->addFilter("path_regex", "\\.o$", true);
  verbose = 3;
  if (path->createList("test2")) {
    cout << "file list is empty" << endl;
    return 0;
  }
  verbose = 4;
  cout << ">List " << path->list()->size() << " file(s):" << endl;
  for (list<File>::iterator i = path->list()->begin();
    i != path->list()->end(); i++) {
    cout << i->line(true) << endl;
  }

  if ((status = db.parse("file://client", "/home/user2", "test2",
      path->list()))) {
    printf("db.parse error status %u\n", status);
    db.close();
    return 0;
  }
  journal.load("test_db", "seen.journal");
  cout << "Added journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  journal.load("test_db", "gone.journal");
  cout << "Removed journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  journal.load("test_db", "written.journal");
  cout << "Really added journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  cout << "Active list: " << db.active()->size() << " element(s):\n";
  for (i = db.active()->begin(); i != db.active()->end(); i++) {
    cout << i->line(true) << endl;
  }
  cout << "Removed list: " << db.removed()->size() << " element(s):\n";
  for (i = db.removed()->begin(); i != db.removed()->end(); i++) {
    cout << i->line(true) << endl;
  }
  delete path;

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

  Directory("test_db/data/59ca0efa9f5633cb0371bbc0355478d8-0").create();
  File2("test_db/data/59ca0efa9f5633cb0371bbc0355478d8-0/data").create();
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
  cout << "Active list: " << db.active()->size() << " element(s):\n";
  for (i = db.active()->begin(); i != db.active()->end(); i++) {
    cout << i->line(true) << endl;
  }
  cout << "Removed list: " << db.removed()->size() << " element(s):\n";
  for (i = db.removed()->begin(); i != db.removed()->end(); i++) {
    cout << i->line(true) << endl;
  }

  system("chmod 0775 test1/testdir");
  system("chmod 0775 test1/cvs/dirutd/CVS/Entries");

  path = new Path("");
  path->addParser("c", "cvs");
  path->addFilter("type", "dir");
  path->addFilter("path_start", ".svn", true);
  path->addFilter("type", "dir");
  path->addFilter("path_start", "subdir", true);
  path->addFilter("type", "file");
  path->addFilter("path_end", "~", true);
  path->addFilter("type", "file");
  path->addFilter("path_regex", "\\.o$", true);
  verbose = 3;
  if (path->createList("test1////")) {
    cout << "file list is empty" << endl;
    return 0;
  }
  verbose = 4;
  cout << ">List " << path->list()->size() << " file(s):" << endl;
  for (list<File>::iterator i = path->list()->begin();
    i != path->list()->end(); i++) {
    cout << i->line(true) << endl;
  }

  if ((status = db.parse("file://host", "/home/user", "test1", path->list()))){
    printf("db.parse error status %u\n", status);
    db.close();
    return 0;
  }
  journal.load("test_db", "seen.journal");
  cout << "Added journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  journal.load("test_db", "gone.journal");
  cout << "Removed journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  journal.load("test_db", "written.journal");
  cout << "Really added journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  cout << "Active list: " << db.active()->size() << " element(s):\n";
  for (i = db.active()->begin(); i != db.active()->end(); i++) {
    cout << i->line(true) << endl;
  }
  cout << "Removed list: " << db.removed()->size() << " element(s):\n";
  for (i = db.removed()->begin(); i != db.removed()->end(); i++) {
    cout << i->line(true) << endl;
  }

  remove("test1/dir space/file space");
  verbose = 3;
  if (path->createList("test1////")) {
    cout << "file list is empty" << endl;
    return 0;
  }
  verbose = 4;
  cout << ">List " << path->list()->size() << " file(s):" << endl;
  for (list<File>::iterator i = path->list()->begin();
    i != path->list()->end(); i++) {
    cout << i->line(true) << endl;
  }

  if ((status = db.parse("file://host", "/home/user", "test1", path->list()))){
    printf("db.parse error status %u\n", status);
    db.close();
    return 0;
  }
  journal.load("test_db", "seen.journal");
  cout << "Added journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  journal.load("test_db", "gone.journal");
  cout << "Removed journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  journal.load("test_db", "written.journal");
  cout << "Really added journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  cout << "Active list: " << db.active()->size() << " element(s):\n";
  for (i = db.active()->begin(); i != db.active()->end(); i++) {
    cout << i->line(true) << endl;
  }
  cout << "Removed list: " << db.removed()->size() << " element(s):\n";
  for (i = db.removed()->begin(); i != db.removed()->end(); i++) {
    cout << i->line(true) << endl;
  }

  db.close();



  /* Re-open database */
  if ((status = db.open())) {
    printf("db_open error status %u\n", status);
    if (status == 2) {
      return 0;
    }
  }
  cout << "Active list: " << db.active()->size() << " element(s):\n";
  for (i = db.active()->begin(); i != db.active()->end(); i++) {
    cout << i->line(true) << endl;
  }
  cout << "Removed list: " << db.removed()->size() << " element(s):\n";
  for (i = db.removed()->begin(); i != db.removed()->end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.load("test_db", "removed");
  cout << "Official removed list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();

  system("chmod 0777 test1/testdir");
  system("chmod 0777 test1/cvs/dirutd/CVS/Entries");

  path = new Path("");
  path->addParser("c", "cvs");
  path->addFilter("type", "dir");
  path->addFilter("path_start", ".svn", true);
  path->addFilter("type", "dir");
  path->addFilter("path_start", "subdir", true);
  path->addFilter("type", "file");
  path->addFilter("path_end", "~", true);
  path->addFilter("type", "file");
  path->addFilter("path_regex", "\\.o$", true);
  verbose = 3;
  if (path->createList("test1////")) {
    cout << "file list is empty" << endl;
    return 0;
  }
  verbose = 4;
  cout << ">List " << path->list()->size() << " file(s):" << endl;
  for (list<File>::iterator i = path->list()->begin();
    i != path->list()->end(); i++) {
    cout << i->line(true) << endl;
  }

  if ((status = db.parse("file://host", "/home/user", "test1", path->list()))){
    printf("db.parse error status %u\n", status);
    db.close();
    return 0;
  }
  journal.load("test_db", "seen.journal");
  cout << "Added journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  journal.load("test_db", "gone.journal");
  cout << "Removed journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  journal.load("test_db", "written.journal");
  cout << "Really added journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  cout << "Active list: " << db.active()->size() << " element(s):\n";
  for (i = db.active()->begin(); i != db.active()->end(); i++) {
    cout << i->line(true) << endl;
  }
  cout << "Removed list: " << db.removed()->size() << " element(s):\n";
  for (i = db.removed()->begin(); i != db.removed()->end(); i++) {
    cout << i->line(true) << endl;
  }

  remove("test1/dir space/file space");
  verbose = 3;
  if (path->createList("test1////")) {
    cout << "file list is empty" << endl;
    return 0;
  }
  verbose = 4;
  cout << ">List " << path->list()->size() << " file(s):" << endl;
  for (list<File>::iterator i = path->list()->begin();
    i != path->list()->end(); i++) {
    cout << i->line(true) << endl;
  }

  if ((status = db.parse("file://host", "/home/user", "test1", path->list()))){
    printf("db.parse error status %u\n", status);
    db.close();
    return 0;
  }
  journal.load("test_db", "seen.journal");
  cout << "Added journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  journal.load("test_db", "gone.journal");
  cout << "Removed journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  journal.load("test_db", "written.journal");
  cout << "Really added journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  cout << "Active list: " << db.active()->size() << " element(s):\n";
  for (i = db.active()->begin(); i != db.active()->end(); i++) {
    cout << i->line(true) << endl;
  }
  cout << "Removed list: " << db.removed()->size() << " element(s):\n";
  for (i = db.removed()->begin(); i != db.removed()->end(); i++) {
    cout << i->line(true) << endl;
  }

  // Test recovery
  cout << "Recovery test" << endl;
  system("head -2 test_db/written.journal > tmp");
  system("mv tmp test_db/written.journal");
  system("echo blah >> test_db/written.journal");
  journal.load("test_db", "written.journal");
  cout << "Really added journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();

  // Simulate crash
  system("echo 1234567890 > test_db/lock");



  /* Re-open database => remove some files */
  if ((status = db.open())) {
    printf("db_open error status %u\n", status);
    if (status == 2) {
      return 0;
    }
  }
  cout << "Active list: " << db.active()->size() << " element(s):\n";
  for (i = db.active()->begin(); i != db.active()->end(); i++) {
    cout << i->line(true) << endl;
  }
  cout << "Removed list: " << db.removed()->size() << " element(s):\n";
  for (i = db.removed()->begin(); i != db.removed()->end(); i++) {
    cout << i->line(true) << endl;
  }

  remove("test1/testfile");
  remove("test1/cvs/dirutd/fileutd");

  path = new Path("");
  path->addParser("c", "cvs");
  path->addFilter("type", "dir");
  path->addFilter("path_start", ".svn", true);
  path->addFilter("type", "dir");
  path->addFilter("path_start", "subdir", true);
  path->addFilter("type", "file");
  path->addFilter("path_end", "~", true);
  path->addFilter("type", "file");
  path->addFilter("path_regex", "\\.o$", true);
  verbose = 3;
  if (path->createList("test1////")) {
    cout << "file list is empty" << endl;
    return 0;
  }
  verbose = 4;
  cout << ">List " << path->list()->size() << " file(s):" << endl;
  for (list<File>::iterator i = path->list()->begin();
    i != path->list()->end(); i++) {
    cout << i->line(true) << endl;
  }

  if ((status = db.parse("file://host", "/home/user", "test1",
      path->list()))) {
    printf("db.parse error status %u\n", status);
    db.close();
    return 0;
  }
  journal.load("test_db", "seen.journal");
  cout << "Added journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  journal.load("test_db", "gone.journal");
  cout << "Removed journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  journal.load("test_db", "written.journal");
  cout << "Really added journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  cout << "Active list: " << db.active()->size() << " element(s):\n";
  for (i = db.active()->begin(); i != db.active()->end(); i++) {
    cout << i->line(true) << endl;
  }
  cout << "Removed list: " << db.removed()->size() << " element(s):\n";
  for (i = db.removed()->begin(); i != db.removed()->end(); i++) {
    cout << i->line(true) << endl;
  }
  delete path;

  path = new Path("");
  path->addParser("c", "cvs");
  path->addFilter("type", "dir");
  path->addFilter("path_start", ".svn", true);
  path->addFilter("type", "dir");
  path->addFilter("path_start", "subdir", true);
  path->addFilter("type", "file");
  path->addFilter("path_end", "~", true);
  path->addFilter("type", "file");
  path->addFilter("path_regex", "\\.o$", true);
  verbose = 3;
  if (path->createList("test1////")) {
    cout << "file list is empty" << endl;
    return 0;
  }
  verbose = 4;
  cout << ">List " << path->list()->size() << " file(s):" << endl;
  for (list<File>::iterator i = path->list()->begin();
    i != path->list()->end(); i++) {
    cout << i->line(true) << endl;
  }

  if ((status = db.parse("file://host", "/home/user", "test1",
      path->list()))) {
    printf("db.parse error status %u\n", status);
    db.close();
    return 0;
  }
  journal.load("test_db", "seen.journal");
  cout << "Added journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  journal.load("test_db", "gone.journal");
  cout << "Removed journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  journal.load("test_db", "written.journal");
  cout << "Really added journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  cout << "Active list: " << db.active()->size() << " element(s):\n";
  for (i = db.active()->begin(); i != db.active()->end(); i++) {
    cout << i->line(true) << endl;
  }
  cout << "Removed list: " << db.removed()->size() << " element(s):\n";
  for (i = db.removed()->begin(); i != db.removed()->end(); i++) {
    cout << i->line(true) << endl;
  }
  delete path;

  path = new Path("");
  path->addParser("c", "cvs");
  path->addFilter("type", "dir");
  path->addFilter("path_start", ".svn", true);
  path->addFilter("type", "dir");
  path->addFilter("path_start", "subdir", true);
  path->addFilter("type", "file");
  path->addFilter("path_end", "~", true);
  path->addFilter("type", "file");
  path->addFilter("path_regex", "\\.o$", true);
  verbose = 3;
  if (path->createList("test2")) {
    cout << "file list is empty" << endl;
    return 0;
  }
  verbose = 4;
  cout << ">List " << path->list()->size() << " file(s):" << endl;
  for (list<File>::iterator i = path->list()->begin();
    i != path->list()->end(); i++) {
    cout << i->line(true) << endl;
  }

  if ((status = db.parse("file://client", "/home/user2", "test2",
      path->list()))) {
    printf("db.parse error status %u\n", status);
    db.close();
    return 0;
  }
  journal.load("test_db", "seen.journal");
  cout << "Added journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  journal.load("test_db", "gone.journal");
  cout << "Removed journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  journal.load("test_db", "written.journal");
  cout << "Really added journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  cout << "Active list: " << db.active()->size() << " element(s):\n";
  for (i = db.active()->begin(); i != db.active()->end(); i++) {
    cout << i->line(true) << endl;
  }
  cout << "Removed list: " << db.removed()->size() << " element(s):\n";
  for (i = db.removed()->begin(); i != db.removed()->end(); i++) {
    cout << i->line(true) << endl;
  }
  delete path;

  db.organise("test_db/data", 2);

  journal.load("test_db", "removed");
  cout << "Official removed list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();

  db.close();



  // Test list uncluttering
  cout << "List uncluttering test" << endl;
  if ((status = db.open())) {
    printf("db_open error status %u\n", status);
    if (status == 2) {
      return 0;
    }
  }
  journal.load("test_db", "removed");
  cout << "Original removed list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  i = journal.begin();
  // Add second element again, different times and checksum
  i++;
  db_data = new DbData(*i->data());
  db_data->setPrefix(i->prefix());
  db_data->setChecksum("BLAH");
  db_data->setOut();
  journal.push_back(*db_data);
  // Add third element again, different times
  i++;
  db_data = new DbData(*i->data());
  db_data->setPrefix(i->prefix());
  db_data->setChecksum(i->checksum());
  db_data->setOut();
  journal.push_back(*db_data);
  // Add third element again, different times and checksum
  db_data = new DbData(*i->data());
  db_data->setPrefix(i->prefix());
  db_data->setChecksum("BLIH");
  db_data->setOut();
  journal.push_back(*db_data);
  // Add third element again, different times
  db_data = new DbData(*i->data());
  db_data->setPrefix(i->prefix());
  db_data->setChecksum(i->checksum());
  db_data->setOut();
  journal.push_back(*db_data);
  // Add fourth element again, different time out
  i++;
  db_data = new DbData(*i);
  db_data->setPrefix(i->prefix());
  db_data->setOut();
  journal.push_back(*db_data);
  // Add fifth element again, different times
  i++;
  db_data = new DbData(*i->data());
  db_data->setPrefix(i->prefix());
  db_data->setChecksum(i->checksum());
  db_data->setOut();
  journal.push_back(*db_data);
  // Add sixth element again
  i++;
  journal.push_back(*i);
  // Add eigth element again, different times
  i++;
  i++;
  db_data = new DbData(*i->data());
  db_data->setPrefix(i->prefix());
  db_data->setChecksum(i->checksum());
  db_data->setOut();
  journal.push_back(*db_data);
  // Add eigth element again, different times and checksum
  db_data = new DbData(*i->data());
  db_data->setPrefix(i->prefix());
  db_data->setChecksum("BLOH");
  db_data->setOut();
  journal.push_back(*db_data);
  // Add eigth element again, different times
  db_data = new DbData(*i->data());
  db_data->setPrefix(i->prefix());
  db_data->setChecksum(i->checksum());
  db_data->setOut();
  journal.push_back(*db_data);
  // Add eigth element again, different times
  db_data = new DbData(*i->data());
  db_data->setPrefix(i->prefix());
  db_data->setChecksum(i->checksum());
  db_data->setOut();
  journal.push_back(*db_data);
  // Sort and show
  journal.sort();
  cout << "Cluttered removed list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  // Save
  journal.save("test_db", "clutter");
  journal.clear();
  // Load (includes sorting and uncluttering)
  journal.load("test_db", "clutter");
  // Show
  cout << "Uncluttered removed list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  db.close();



  /* Re-open database => one less active item */
  if ((status = db.open())) {
    printf("db_open error status %u\n", status);
    if (status == 2) {
      return 0;
    }
  }
  cout << "Active list: " << db.active()->size() << " element(s):\n";
  for (i = db.active()->begin(); i != db.active()->end(); i++) {
    cout << i->line(true) << endl;
  }
  cout << "Removed list: " << db.removed()->size() << " element(s):\n";
  for (i = db.removed()->begin(); i != db.removed()->end(); i++) {
    cout << i->line(true) << endl;
  }

  cout << endl << "Test: getdir" << endl;
  cout << "Check test_db/data dir: " << ! Directory("test_db/data").isValid() << endl;
  File2("test_db/data/.nofiles").create();
  Directory("test_db/data/fe").create();
  File2("test_db/data/fe/.nofiles").create();
  File2("test_db/data/fe/test4").create();
  Directory("test_db/data/fe/dc").create();
  File2("test_db/data/fe/dc/.nofiles").create();
  Directory("test_db/data/fe/ba").create();
  Directory("test_db/data/fe/ba/test1").create();
  Directory("test_db/data/fe/98").create();
  Directory("test_db/data/fe/98/test2").create();
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
  Directory("test_db/data/fe/dc/76").create();
  cout << "fedc76test6 status: " << db.getDir("fedc76test6", getdir_path, true)
    << ", getdir_path: " << getdir_path << endl;
  mkdir("test_db/data/fe/dc/76/test6", 0755);
  cout << "fedc76test6 status: " << db.getDir("fedc76test6", getdir_path, true)
    << ", getdir_path: " << getdir_path << endl;

  // Test expiration
  cout << "Expiration test" << endl;
  db.expire_share("file://host", "/home/user", 1);
  db.expire_finalise();
  // Modify the specific record I need
  journal.load("test_db", "removed");
  journal.back().setOut(1);
  cout << "Modified removed list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.save("test_db", "removed");
  journal.clear();
  db.expire_init();
  db.close_active();
  db.open_removed();
  db.expire_share("file://host", "/home/user", 30);
  db.expire_finalise();

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
  system("echo blah >> test_db/active");
  if ((status = db.open())) {
    printf("Error: %s\n", strerror(errno));
  }

  cout << "List is gone" << endl;
  remove("test_db/active");
  remove("test_db/active~");
  if ((status = db.open())) {
    printf("Error: %s\n", strerror(errno));
  }

  return 0;
}