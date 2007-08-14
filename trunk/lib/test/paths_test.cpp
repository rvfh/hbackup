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
#include "paths.h"

using namespace hbackup;

int verbosity(void) {
  return 3;
}

int terminating(void) {
  return 0;
}

void showList(const Directory* d, const char* path = "");

void showFile(const Node* g, const char* path = "") {
  printf("%s%s\t%c\t%lld\t%d\t%d\t%d\t%o\n", path, g->name(), g->type(),
    g->size(), (g->mtime() != 0), g->uid(), g->gid(), g->mode());
  if (g->type() == 'd') {
    char* dir_path = NULL;
    asprintf(&dir_path, "%s%s/", path, g->name());
    Directory* d = (Directory*) g;
    showList(d, dir_path);
    free(dir_path);
  }
}

void showList(const Directory* d, const char* path) {
  list<Node*>::const_iterator i;
  for (i = d->nodesListConst().begin(); i != d->nodesListConst().end(); i++) {
    showFile(*i, path);
  }
}

int main(void) {
  Path* path = new Path("");

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
  cout << endl << "New classes test" << endl;
  Path2* path2 = new Path2("");

  if (! path2->parse("test1")) {
    cout << ">List " << path2->nodes() << " file(s):\n";
    showList(path2->dir());
  }

  cout << "as previous with subdir in ignore list" << endl;
  if (path2->addFilter("type", "dir")
   || path2->addFilter("path", "subdir", true)) {
    cout << "Failed to add filter" << endl;
  }
  if (! path2->parse("test1")) {
    cout << ">List " << path2->nodes() << " file(s):\n";
    showList(path2->dir());
  }

  cout << "as previous with testlink in ignore list" << endl;
  if (path2->addFilter("type", "link")
   || path2->addFilter("path_start", "testlink", true)) {
    cout << "Failed to add filter" << endl;
  }
  if (! path2->parse("test1")) {
    cout << ">List " << path2->nodes() << " file(s):\n";
    showList(path2->dir());
  }

  cout << "as previous with CVS parser" << endl;
  if (path2->addParser("cvs", "controlled")) {
    cout << "Failed to add parser" << endl;
  }
  if (! path2->parse("test1")) {
    cout << ">List " << path2->nodes() << " file(s):\n";
    showList(path2->dir());
  }

  cout << "as previous" << endl;
  if (! path2->parse("test1")) {
    cout << ">List " << path2->nodes() << " file(s):\n";
    showList(path2->dir());
  }

  delete path2;

  return 0;
}
