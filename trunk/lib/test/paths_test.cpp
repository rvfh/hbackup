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
#include <vector>
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

int main(void) {
  Path* path = new Path("");

  if (! path->createList("test")) {
    cout << ">List " << path->list()->size() << " file(s):\n";
    for (list<File>::iterator i = path->list()->begin();
     i != path->list()->end(); i++) {
      cout << i->line(true) << endl;
    }
  }

  cout << "as previous with subdir in ignore list" << endl;
  if (path->addFilter("type", "dir")
   || path->addFilter("path_start", "subdir", true)) {
    cout << "Failed to add filter" << endl;
  }
  if (! path->createList("test")) {
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
  if (! path->createList("test")) {
    cout << ">List " << path->list()->size() << " file(s):\n";
    for (list<File>::iterator i = path->list()->begin();
     i != path->list()->end(); i++) {
      cout << i->line(true) << endl;
    }
  }

  cout << "as previous with CVS parser" << endl;
  if (path->addParser("controlled", "cvs")) {
    cout << "Failed to add parser" << endl;
  }
  if (! path->createList("test")) {
    cout << ">List " << path->list()->size() << " file(s):\n";
    for (list<File>::iterator i = path->list()->begin();
     i != path->list()->end(); i++) {
      cout << i->line(true) << endl;
    }
  }

  cout << "as previous" << endl;
  if (! path->createList("test")) {
    cout << ">List " << path->list()->size() << " file(s):\n";
    for (list<File>::iterator i = path->list()->begin();
     i != path->list()->end(); i++) {
      cout << i->line(true) << endl;
    }
  }

  delete path;
  return 0;
}
