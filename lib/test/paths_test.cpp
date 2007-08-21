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
#include "dbdata.h"
#include "dblist.h"
#include "db.h"
#include "paths.h"

using namespace hbackup;

int verbosity(void) {
  return 4;
}

int terminating(void) {
  return 0;
}

static time_t my_time = 0;

time_t time(time_t *t) {
  return my_time;
}

void showLine(time_t timestamp, char* prefix, char* path, Node* node) {
  printf("[%2ld] %-16s %-34s", timestamp, prefix, path);
  if (node != NULL) {
    printf(" %c %5llu %03o", node->type(), node->size(), node->mode());
    if (node->type() == 'f') {
      printf(" %s", ((File*) node)->checksum());
    }
    if (node->type() == 'l') {
      printf(" %s", ((Link*) node)->link());
    }
  } else {
    printf(" [rm]");
  }
  cout << endl;
}

int main(void) {
  Path* path = new Path("/home/User");
  Database  db("test_db");
  // Journal
  List    journal("test_db", "journal");
  time_t  timestamp;
  char*   prefix;
  char*   fpath;
  Node*   node;

  // Initialisation
  my_time++;
  db.open();

  // Display DB contents
  cout << "Active list:  " << ((DbList*)db.active())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.active())->begin();
       i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }
  db.open_removed();
  cout << "Removed list: " << ((DbList*)db.removed())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.removed())->begin();
       i != ((DbList*)db.removed())->end(); i++) {
    i->line();
  }

  // '-' is before '/' in the ASCII table...
  system("touch test1/subdir-file");
  system("touch test1/subdirfile");
  system("touch test1/àccénts_test");

  cout << "first with subdir/testfile NOT readable" << endl;
  system("chmod 000 test1/subdir/testfile");
  if (! path->parse(db, "file://localhost", "test1")) {
//     cout << ">List " << path->nodes() << " file(s):\n";
  }

  // Display DB contents
  cout << "Active list:  " << ((DbList*)db.active())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.active())->begin();
       i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }
  db.open_removed();
  cout << "Removed list: " << ((DbList*)db.removed())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.removed())->begin();
       i != ((DbList*)db.removed())->end(); i++) {
    i->line();
  }

  db.close();
  // Show journal contents
  if (! journal.open("r")) {
    while (journal.getLine(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
      free(prefix);
      free(fpath);
      free(node);
    }
    journal.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }

  // Next test
  my_time++;
  db.open();

  cout << "as previous" << endl;
  if (! path->parse(db, "file://localhost", "test1")) {
//     cout << ">List " << path->nodes() << " file(s):\n";
  }

  // Display DB contents
  cout << "Active list:  " << ((DbList*)db.active())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.active())->begin();
       i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }
  db.open_removed();
  cout << "Removed list: " << ((DbList*)db.removed())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.removed())->begin();
       i != ((DbList*)db.removed())->end(); i++) {
    i->line();
  }

  db.close();
  // Show journal contents
  if (! journal.open("r")) {
    while (journal.getLine(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
      free(prefix);
      free(fpath);
      free(node);
    }
    journal.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }

  // Next test
  my_time++;
  db.open();

  cout << "as previous with subdir/testfile readable" << endl;
  system("chmod 644 test1/subdir/testfile");
  if (! path->parse(db, "file://localhost", "test1")) {
//     cout << ">List " << path->nodes() << " file(s):\n";
  }

  // Display DB contents
  cout << "Active list:  " << ((DbList*)db.active())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.active())->begin();
       i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }
  db.open_removed();
  cout << "Removed list: " << ((DbList*)db.removed())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.removed())->begin();
       i != ((DbList*)db.removed())->end(); i++) {
    i->line();
  }

  db.close();
  // Show journal contents
  if (! journal.open("r")) {
    while (journal.getLine(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
      free(prefix);
      free(fpath);
      free(node);
    }
    journal.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }

  // Next test
  my_time++;
  db.open();

  cout << "as previous with subdir/testfile in ignore list" << endl;
  if (path->addFilter("type", "file")
   || path->addFilter("path", "subdir/testfile", true)) {
    cout << "Failed to add filter" << endl;
  }
  if (! path->parse(db, "file://localhost", "test1")) {
//     cout << ">List " << path->nodes() << " file(s):\n";
  }

  // Display DB contents
  cout << "Active list:  " << ((DbList*)db.active())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.active())->begin();
       i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }
  db.open_removed();
  cout << "Removed list: " << ((DbList*)db.removed())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.removed())->begin();
       i != ((DbList*)db.removed())->end(); i++) {
    i->line();
  }

  db.close();
  // Show journal contents
  if (! journal.open("r")) {
    while (journal.getLine(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
      free(prefix);
      free(fpath);
      free(node);
    }
    journal.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }

  // Next test
  my_time++;
  db.open();

  cout << "as previous with subdir in ignore list" << endl;
  if (path->addFilter("type", "dir")
   || path->addFilter("path", "subdir", true)) {
    cout << "Failed to add filter" << endl;
  }
  if (! path->parse(db, "file://localhost", "test1")) {
//     cout << ">List " << path->nodes() << " file(s):\n";
  }

  // Display DB contents
  cout << "Active list:  " << ((DbList*)db.active())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.active())->begin();
       i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }
  db.open_removed();
  cout << "Removed list: " << ((DbList*)db.removed())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.removed())->begin();
       i != ((DbList*)db.removed())->end(); i++) {
    i->line();
  }

  db.close();
  // Show journal contents
  if (! journal.open("r")) {
    while (journal.getLine(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
      free(prefix);
      free(fpath);
      free(node);
    }
    journal.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }

  // Next test
  my_time++;
  db.open();

  cout << "as previous with testlink modified" << endl;
  system("sleep 1 && ln -sf testnull test1/testlink");
  if (path->addFilter("type", "dir")
   || path->addFilter("path", "subdir", true)) {
    cout << "Failed to add filter" << endl;
  }
  if (! path->parse(db, "file://localhost", "test1")) {
//     cout << ">List " << path->nodes() << " file(s):\n";
  }

  // Display DB contents
  cout << "Active list:  " << ((DbList*)db.active())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.active())->begin();
       i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }
  db.open_removed();
  cout << "Removed list: " << ((DbList*)db.removed())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.removed())->begin();
       i != ((DbList*)db.removed())->end(); i++) {
    i->line();
  }

  db.close();
  // Show journal contents
  if (! journal.open("r")) {
    while (journal.getLine(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
      free(prefix);
      free(fpath);
      free(node);
    }
    journal.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }

  // Next test
  my_time++;
  db.open();

  cout << "as previous with testlink in ignore list" << endl;
  if (path->addFilter("type", "link")
   || path->addFilter("path_start", "testlink", true)) {
    cout << "Failed to add filter" << endl;
  }
  if (! path->parse(db, "file://localhost", "test1")) {
//     cout << ">List " << path->nodes() << " file(s):\n";
  }

  // Display DB contents
  cout << "Active list:  " << ((DbList*)db.active())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.active())->begin();
       i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }
  db.open_removed();
  cout << "Removed list: " << ((DbList*)db.removed())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.removed())->begin();
       i != ((DbList*)db.removed())->end(); i++) {
    i->line();
  }

  db.close();
  // Show journal contents
  if (! journal.open("r")) {
    while (journal.getLine(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
      free(prefix);
      free(fpath);
      free(node);
    }
    journal.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }

  // Next test
  my_time++;
  db.open();

  cout << "as previous with CVS parser" << endl;
  if (path->addParser("cvs", "controlled")) {
    cout << "Failed to add parser" << endl;
  }
  if (! path->parse(db, "file://localhost", "test1")) {
//     cout << ">List " << path->nodes() << " file(s):\n";
  }

  // Display DB contents
  cout << "Active list:  " << ((DbList*)db.active())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.active())->begin();
       i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }
  db.open_removed();
  cout << "Removed list: " << ((DbList*)db.removed())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.removed())->begin();
       i != ((DbList*)db.removed())->end(); i++) {
    i->line();
  }

  db.close();
  // Show journal contents
  if (! journal.open("r")) {
    while (journal.getLine(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
      free(prefix);
      free(fpath);
      free(node);
    }
    journal.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }

  // Next test
  my_time++;
  db.open();

  cout << "as previous" << endl;
  if (! path->parse(db, "file://localhost", "test1")) {
//     cout << ">List " << path->nodes() << " file(s):\n";
  }

  // Display DB contents
  cout << "Active list:  " << ((DbList*)db.active())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.active())->begin();
       i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }
  db.open_removed();
  cout << "Removed list: " << ((DbList*)db.removed())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.removed())->begin();
       i != ((DbList*)db.removed())->end(); i++) {
    i->line();
  }

  db.close();
  // Show journal contents
  if (! journal.open("r")) {
    while (journal.getLine(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
      free(prefix);
      free(fpath);
      free(node);
    }
    journal.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }

  // Next test
  my_time++;
  db.open();

  cout << "as previous with cvs/dirutd in ignore list" << endl;
  if (path->addFilter("type", "dir")
   || path->addFilter("path", "cvs/dirutd", true)) {
    cout << "Failed to add filter" << endl;
  }
  if (! path->parse(db, "file://localhost", "test1")) {
//     cout << ">List " << path->nodes() << " file(s):\n";
  }

  // Display DB contents
  cout << "Active list:  " << ((DbList*)db.active())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.active())->begin();
       i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }
  db.open_removed();
  cout << "Removed list: " << ((DbList*)db.removed())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.removed())->begin();
       i != ((DbList*)db.removed())->end(); i++) {
    i->line();
  }

  db.close();
  // Show journal contents
  if (! journal.open("r")) {
    while (journal.getLine(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
      free(prefix);
      free(fpath);
      free(node);
    }
    journal.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }

  // Next test
  my_time++;
  db.open();

  cout << "as previous with testpipe gone" << endl;
  remove("test1/testpipe");
  if (! path->parse(db, "file://localhost", "test1")) {
//     cout << ">List " << path->nodes() << " file(s):\n";
  }

  // Display DB contents
  cout << "Active list:  " << ((DbList*)db.active())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.active())->begin();
       i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }
  db.open_removed();
  cout << "Removed list: " << ((DbList*)db.removed())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.removed())->begin();
       i != ((DbList*)db.removed())->end(); i++) {
    i->line();
  }

  db.close();
  // Show journal contents
  if (! journal.open("r")) {
    while (journal.getLine(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
      free(prefix);
      free(fpath);
      free(node);
    }
    journal.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }

  // Next test
  my_time++;
  db.open();

  cout << "as previous with testfile mode changed" << endl;
  system("chmod 660 test1/testfile");
  if (! path->parse(db, "file://localhost", "test1")) {
//     cout << ">List " << path->nodes() << " file(s):\n";
  }

  // Display DB contents
  cout << "Active list:  " << ((DbList*)db.active())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.active())->begin();
       i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }
  db.open_removed();
  cout << "Removed list: " << ((DbList*)db.removed())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.removed())->begin();
       i != ((DbList*)db.removed())->end(); i++) {
    i->line();
  }

  db.close();
  // Show journal contents
  if (! journal.open("r")) {
    while (journal.getLine(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
      free(prefix);
      free(fpath);
      free(node);
    }
    journal.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }

  // Next test
  my_time++;
  db.open();

  cout << "as previous with cvs/filenew.c touched" << endl;
  system("echo blah > test1/cvs/filenew.c");
  if (! path->parse(db, "file://localhost", "test1")) {
//     cout << ">List " << path->nodes() << " file(s):\n";
  }

  // Display DB contents
  cout << "Active list:  " << ((DbList*)db.active())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.active())->begin();
       i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }
  db.open_removed();
  cout << "Removed list: " << ((DbList*)db.removed())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.removed())->begin();
       i != ((DbList*)db.removed())->end(); i++) {
    i->line();
  }

  db.close();
  // Show journal contents
  if (! journal.open("r")) {
    while (journal.getLine(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
      free(prefix);
      free(fpath);
      free(node);
    }
    journal.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }

  // Next test
  my_time++;
  db.open();

  cout << "some troublesome past cases" << endl;

  system("mkdir -p test1/docbook-xml/3.1.7");
  system("touch test1/docbook-xml/3.1.7/dbgenent.ent");
  system("mkdir test1/docbook-xml/4.0");
  system("touch test1/docbook-xml/4.0/dbgenent.ent");
  system("mkdir test1/docbook-xml/4.1.2");
  system("touch test1/docbook-xml/4.1.2/dbgenent.mod");
  system("mkdir test1/docbook-xml/4.2");
  system("touch test1/docbook-xml/4.2/dbgenent.mod");
  system("mkdir test1/docbook-xml/4.3");
  system("touch test1/docbook-xml/4.3/dbgenent.mod");
  system("mkdir test1/docbook-xml/4.4");
  system("touch test1/docbook-xml/4.4/dbgenent.mod");
  system("touch test1/docbook-xml.cat");
  system("touch test1/docbook-xml.cat.old");

  system("mkdir -p test1/testdir/biblio");
  system("touch test1/testdir/biblio/biblio.dbf");
  system("touch test1/testdir/biblio/biblio.dbt");
  system("touch test1/testdir/biblio.odb");
  system("touch test1/testdir/evolocal.odb");

  if (! path->parse(db, "file://localhost", "test1")) {
//     cout << ">List " << path->nodes() << " file(s):\n";
  }

  // Display DB contents
  cout << "Active list:  " << ((DbList*)db.active())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.active())->begin();
       i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }
  db.open_removed();
  cout << "Removed list: " << ((DbList*)db.removed())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.removed())->begin();
       i != ((DbList*)db.removed())->end(); i++) {
    i->line();
  }

  db.close();
  // Show journal contents
  if (! journal.open("r")) {
    while (journal.getLine(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
      free(prefix);
      free(fpath);
      free(node);
    }
    journal.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }

  // Next test
  my_time++;
  db.open();

  if (! path->parse(db, "file://localhost", "test1")) {
//     cout << ">List " << path->nodes() << " file(s):\n";
  }

  // Display DB contents
  cout << "Active list:  " << ((DbList*)db.active())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.active())->begin();
       i != ((DbList*)db.active())->end(); i++) {
    i->line();
  }
  db.open_removed();
  cout << "Removed list: " << ((DbList*)db.removed())->size()
    << " element(s):\n";
  for (DbList::iterator i = ((DbList*)db.removed())->begin();
       i != ((DbList*)db.removed())->end(); i++) {
    i->line();
  }

  delete path;
  db.close();
  // Show journal contents
  if (! journal.open("r")) {
    while (journal.getLine(&timestamp, &prefix, &fpath, &node) > 0) {
      showLine(timestamp, prefix, fpath, node);
      free(prefix);
      free(fpath);
      free(node);
    }
    journal.close();
  } else {
    cerr << "Failed to open journal" << endl;
  }

  return 0;
}
