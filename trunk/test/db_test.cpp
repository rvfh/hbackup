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

#include "files.h"
#include "filters.h"
#include "parsers.h"
#include "cvs_parser.h"
#include "paths.h"
#include "list.h"
#include "db.h"

static int verbose = 3;

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
  DbData*   db_data;
  off_t     size;
  off_t     zsize;
  SortedList<DbData>::iterator i;
  SortedList<DbData> journal;
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
  path->addParser("c", "cvs");
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
  db_data = new DbData(File("test/testfile"));
  if ((status = db.write("test/testfile", *db_data))) {
    printf("db.write error status %u\n", status);
    db.close();
    return 0;
  }
  cout << db_data->checksum() << "  test/testfile" << endl;

  /* Read check */
  if ((status = db.read("test_db/blah", db_data->checksum()))) {
    printf("db.read error status %u\n", status);
    db.close();
    return 0;
  }

  if ((status = db.parse("file://host", "/home/user", "test", path->list()))) {
    printf("db.parse error status %u\n", status);
    db.close();
    return 0;
  }
  db.load("added.journal", journal);
  cout << "Added journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  db.load("removed.journal", journal);
  cout << "Removed journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  db.load("written.journal", journal);
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

  if ((status = db.parse("file://host", "/home/user", "test", path->list()))) {
    printf("db.parse error status %u\n", status);
    db.close();
    return 0;
  }
  db.load("added.journal", journal);
  cout << "Added journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  db.load("removed.journal", journal);
  cout << "Removed journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  db.load("written.journal", journal);
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
  path->addFilter("path_regexp", "\\.o$", true);
  if (path->createList("test2")) {
    cout << "file list is empty" << endl;
    return 0;
  }
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
  db.load("added.journal", journal);
  cout << "Added journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  db.load("removed.journal", journal);
  cout << "Removed journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  db.load("written.journal", journal);
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

  system("chmod 0775 test/testdir");
  system("chmod 0775 test/cvs/dirutd/CVS/Entries");

  path = new Path("");
  path->addParser("c", "cvs");
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
    cout << i->line(true) << endl;
  }

  if ((status = db.parse("file://host", "/home/user", "test", path->list()))) {
    printf("db.parse error status %u\n", status);
    db.close();
    return 0;
  }
  db.load("added.journal", journal);
  cout << "Added journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  db.load("removed.journal", journal);
  cout << "Removed journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  db.load("written.journal", journal);
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

  remove("test/dir space/file space");
  if (path->createList("test////")) {
    cout << "file list is empty" << endl;
    return 0;
  }
  cout << ">List " << path->list()->size() << " file(s):" << endl;
  for (list<File>::iterator i = path->list()->begin();
    i != path->list()->end(); i++) {
    cout << i->line(true) << endl;
  }

  if ((status = db.parse("file://host", "/home/user", "test", path->list()))) {
    printf("db.parse error status %u\n", status);
    db.close();
    return 0;
  }
  db.load("added.journal", journal);
  cout << "Added journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  db.load("removed.journal", journal);
  cout << "Removed journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  db.load("written.journal", journal);
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
  db.load("removed", journal);
  cout << "Official removed list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();

  system("chmod 0777 test/testdir");
  system("chmod 0777 test/cvs/dirutd/CVS/Entries");

  path = new Path("");
  path->addParser("c", "cvs");
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
    cout << i->line(true) << endl;
  }

  if ((status = db.parse("file://host", "/home/user", "test", path->list()))) {
    printf("db.parse error status %u\n", status);
    db.close();
    return 0;
  }
  db.load("added.journal", journal);
  cout << "Added journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  db.load("removed.journal", journal);
  cout << "Removed journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  db.load("written.journal", journal);
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

  remove("test/dir space/file space");
  if (path->createList("test////")) {
    cout << "file list is empty" << endl;
    return 0;
  }
  cout << ">List " << path->list()->size() << " file(s):" << endl;
  for (list<File>::iterator i = path->list()->begin();
    i != path->list()->end(); i++) {
    cout << i->line(true) << endl;
  }

  if ((status = db.parse("file://host", "/home/user", "test", path->list()))) {
    printf("db.parse error status %u\n", status);
    db.close();
    return 0;
  }
  db.load("added.journal", journal);
  cout << "Added journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  db.load("removed.journal", journal);
  cout << "Removed journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  db.load("written.journal", journal);
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
  db.load("written.journal", journal);
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

  remove("test/testfile");
  remove("test/cvs/dirutd/fileutd");

  path = new Path("");
  path->addParser("c", "cvs");
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
    cout << i->line(true) << endl;
  }

  if ((status = db.parse("file://host", "/home/user", "test",
      path->list()))) {
    printf("db.parse error status %u\n", status);
    db.close();
    return 0;
  }
  db.load("added.journal", journal);
  cout << "Added journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  db.load("removed.journal", journal);
  cout << "Removed journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  db.load("written.journal", journal);
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
  path->addFilter("path_regexp", "\\.o$", true);
  if (path->createList("test////")) {
    cout << "file list is empty" << endl;
    return 0;
  }
  cout << ">List " << path->list()->size() << " file(s):" << endl;
  for (list<File>::iterator i = path->list()->begin();
    i != path->list()->end(); i++) {
    cout << i->line(true) << endl;
  }

  if ((status = db.parse("file://host", "/home/user", "test",
      path->list()))) {
    printf("db.parse error status %u\n", status);
    db.close();
    return 0;
  }
  db.load("added.journal", journal);
  cout << "Added journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  db.load("removed.journal", journal);
  cout << "Removed journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  db.load("written.journal", journal);
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
  path->addFilter("path_regexp", "\\.o$", true);
  if (path->createList("test2")) {
    cout << "file list is empty" << endl;
    return 0;
  }
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
  db.load("added.journal", journal);
  cout << "Added journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  db.load("removed.journal", journal);
  cout << "Removed journal list: " << journal.size() << " element(s):\n";
  for (i = journal.begin(); i != journal.end(); i++) {
    cout << i->line(true) << endl;
  }
  journal.clear();
  db.load("written.journal", journal);
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

  db.organize("test_db/data", 2);

  db.load("removed", journal);
  cout << "Official removed list: " << journal.size() << " element(s):\n";
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
  system("chmod ugo-r test_db/list");
  if ((status = db.open())) {
    printf("Error: %s\n", strerror(errno));
  }

  cout << "List is garbaged" << endl;
  system("chmod u+r test_db/list");
  system("echo blah >> test_db/list");
  if ((status = db.open())) {
    printf("Error: %s\n", strerror(errno));
  }

  cout << "List is gone" << endl;
  remove("test_db/list");
  if ((status = db.open())) {
    printf("Error: %s\n", strerror(errno));
  }

  return 0;
}
