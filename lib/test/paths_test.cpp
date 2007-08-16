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

// Test add* functions

using namespace std;

#include <iostream>
#include <list>
#include <string>

#include "files.h"
#include "filters.h"
#include "parsers.h"
#include "cvs_parser.h"
#include "list.h"
#include "dbdata.h"
#include "dblist.h"
#include "db.h"
#include "paths.h"

using namespace hbackup;

static int verbose = 3;

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
  Path* path = new Path("/home/User");
  Database  db("test_db");

  if (! path->createList("test1")) {
    cout << ">List " << path->list()->size() << " file(s):\n";
    for (list<File>::iterator i = path->list()->begin();
     i != path->list()->end(); i++) {
      cout << i->line(true) << endl;
    }
  }

  cout << "as previous with subdir in ignore list" << endl;
  if (path->addFilter("type", "dir")
   || path->addFilter("path", "subdir", true)) {
    cout << "Failed to add filter" << endl;
  }
  if (! path->createList("test1")) {
    cout << ">List " << path->list()->size() << " file(s):\n";
    for (list<File>::iterator i = path->list()->begin();
     i != path->list()->end(); i++) {
      cout << i->line(true) << endl;
    }
  }

  cout << "as previous with testlink in ignore list" << endl;
  if (path->addFilter("type", "link")
   || path->addFilter("path_start", "testlink", true)) {
    cout << "Failed to add filter" << endl;
  }
  if (! path->createList("test1")) {
    cout << ">List " << path->list()->size() << " file(s):\n";
    for (list<File>::iterator i = path->list()->begin();
     i != path->list()->end(); i++) {
      cout << i->line(true) << endl;
    }
  }

  cout << "as previous with CVS parser" << endl;
  if (path->addParser("cvs", "controlled")) {
    cout << "Failed to add parser" << endl;
  }
  if (! path->createList("test1")) {
    cout << ">List " << path->list()->size() << " file(s):\n";
    for (list<File>::iterator i = path->list()->begin();
     i != path->list()->end(); i++) {
      cout << i->line(true) << endl;
    }
  }

  cout << "as previous" << endl;
  if (! path->createList("test1")) {
    cout << ">List " << path->list()->size() << " file(s):\n";
    for (list<File>::iterator i = path->list()->begin();
     i != path->list()->end(); i++) {
      cout << i->line(true) << endl;
    }
  }

  delete path;

  // New classes
  verbose = 99;
  db.open();

  // Display DB contents
  cout << "Active list:  " << ((DbList*)db.active())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.active())->begin();
       i != ((DbList*)db.active())->end(); i++) {
    cout << i->line(true) << endl;
  }
  cout << "Removed list: " << ((DbList*)db.removed())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.removed())->begin();
       i != ((DbList*)db.removed())->end(); i++) {
    cout << i->line(true) << endl;
  }

  cout << endl << "New classes test" << endl;
  Path2* path2 = new Path2("/home/User");

  if (! path2->parse(db, "file://localhost", "test1")) {
//     cout << ">List " << path2->nodes() << " file(s):\n";
  }

  // Display DB contents
  cout << "Active list:  " << ((DbList*)db.active())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.active())->begin();
       i != ((DbList*)db.active())->end(); i++) {
    cout << i->line(true) << endl;
  }
  cout << "Removed list: " << ((DbList*)db.removed())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.removed())->begin();
       i != ((DbList*)db.removed())->end(); i++) {
    cout << i->line(true) << endl;
  }

  cout << "as previous with subdir in ignore list" << endl;
  if (path2->addFilter("type", "dir")
   || path2->addFilter("path", "subdir", true)) {
    cout << "Failed to add filter" << endl;
  }
  if (! path2->parse(db, "file://localhost", "test1")) {
//     cout << ">List " << path2->nodes() << " file(s):\n";
  }

  // Display DB contents
  cout << "Active list:  " << ((DbList*)db.active())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.active())->begin();
       i != ((DbList*)db.active())->end(); i++) {
    cout << i->line(true) << endl;
  }
  cout << "Removed list: " << ((DbList*)db.removed())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.removed())->begin();
       i != ((DbList*)db.removed())->end(); i++) {
    cout << i->line(true) << endl;
  }

  cout << "as previous with testlink in ignore list" << endl;
  if (path2->addFilter("type", "link")
   || path2->addFilter("path_start", "testlink", true)) {
    cout << "Failed to add filter" << endl;
  }
  if (! path2->parse(db, "file://localhost", "test1")) {
//     cout << ">List " << path2->nodes() << " file(s):\n";
  }

  // Display DB contents
  cout << "Active list:  " << ((DbList*)db.active())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.active())->begin();
       i != ((DbList*)db.active())->end(); i++) {
    cout << i->line(true) << endl;
  }
  cout << "Removed list: " << ((DbList*)db.removed())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.removed())->begin();
       i != ((DbList*)db.removed())->end(); i++) {
    cout << i->line(true) << endl;
  }

  cout << "as previous with CVS parser" << endl;
  if (path2->addParser("cvs", "controlled")) {
    cout << "Failed to add parser" << endl;
  }
  if (! path2->parse(db, "file://localhost", "test1")) {
//     cout << ">List " << path2->nodes() << " file(s):\n";
  }

  // Display DB contents
  cout << "Active list:  " << ((DbList*)db.active())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.active())->begin();
       i != ((DbList*)db.active())->end(); i++) {
    cout << i->line(true) << endl;
  }
  cout << "Removed list: " << ((DbList*)db.removed())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.removed())->begin();
       i != ((DbList*)db.removed())->end(); i++) {
    cout << i->line(true) << endl;
  }

  cout << "as previous" << endl;
  if (! path2->parse(db, "file://localhost", "test1")) {
//     cout << ">List " << path2->nodes() << " file(s):\n";
  }

  // Display DB contents
  cout << "Active list:  " << ((DbList*)db.active())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.active())->begin();
       i != ((DbList*)db.active())->end(); i++) {
    cout << i->line(true) << endl;
  }
  cout << "Removed list: " << ((DbList*)db.removed())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.removed())->begin();
       i != ((DbList*)db.removed())->end(); i++) {
    cout << i->line(true) << endl;
  }

  cout << "as previous with cvs/dirutd in ignore list" << endl;
  if (path2->addFilter("type", "dir")
   || path2->addFilter("path", "cvs/dirutd", true)) {
    cout << "Failed to add filter" << endl;
  }
  if (! path2->parse(db, "file://localhost", "test1")) {
//     cout << ">List " << path2->nodes() << " file(s):\n";
  }

  // Display DB contents
  cout << "Active list:  " << ((DbList*)db.active())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.active())->begin();
       i != ((DbList*)db.active())->end(); i++) {
    cout << i->line(true) << endl;
  }
  cout << "Removed list: " << ((DbList*)db.removed())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.removed())->begin();
       i != ((DbList*)db.removed())->end(); i++) {
    cout << i->line(true) << endl;
  }

  cout << "as previous with testpipe gone" << endl;
  remove("test1/testpipe");
  if (! path2->parse(db, "file://localhost", "test1")) {
//     cout << ">List " << path2->nodes() << " file(s):\n";
  }

  // Display DB contents
  cout << "Active list:  " << ((DbList*)db.active())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.active())->begin();
       i != ((DbList*)db.active())->end(); i++) {
    cout << i->line(true) << endl;
  }
  cout << "Removed list: " << ((DbList*)db.removed())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.removed())->begin();
       i != ((DbList*)db.removed())->end(); i++) {
    cout << i->line(true) << endl;
  }

  cout << "as previous with testfile mode changed" << endl;
  system("chmod 660 test1/testfile");
  if (! path2->parse(db, "file://localhost", "test1")) {
//     cout << ">List " << path2->nodes() << " file(s):\n";
  }

  // Display DB contents
  cout << "Active list:  " << ((DbList*)db.active())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.active())->begin();
       i != ((DbList*)db.active())->end(); i++) {
    cout << i->line(true) << endl;
  }
  cout << "Removed list: " << ((DbList*)db.removed())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.removed())->begin();
       i != ((DbList*)db.removed())->end(); i++) {
    cout << i->line(true) << endl;
  }

  cout << "as previous with cvs/filenew.c touched" << endl;
  system("echo blah > test1/cvs/filenew.c");
  if (! path2->parse(db, "file://localhost", "test1")) {
//     cout << ">List " << path2->nodes() << " file(s):\n";
  }

  // Display DB contents
  cout << "Active list:  " << ((DbList*)db.active())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.active())->begin();
       i != ((DbList*)db.active())->end(); i++) {
    cout << i->line(true) << endl;
  }
  cout << "Removed list: " << ((DbList*)db.removed())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.removed())->begin();
       i != ((DbList*)db.removed())->end(); i++) {
    cout << i->line(true) << endl;
  }

  delete path2;
  db.close();

  return 0;
}
