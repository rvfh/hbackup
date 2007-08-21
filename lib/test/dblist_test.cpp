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

#include <iostream>
#include <errno.h>

using namespace std;

#include "files.h"
#include "dbdata.h"
#include "dblist.h"

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
  List    journal("test1/journal");
  char*   line   = NULL;
  char*   prefix = NULL;
  char*   path   = NULL;
  Node*   node;
  time_t  ts;

  if (journal.open("w")) {
    cerr << "Failed to open journal" << endl;
    return -1;
  }
  journal.removed("prefix", "path");
  node = new Stream("test1/testfile");
  ((Stream*) node)->computeChecksum();
  journal.added("prefix2", "path2", node, false);
  free(node);
  node = new Link("test1/testlink");
  journal.added("prefix3", "path3", node, true);
  free(node);
  node = new Directory("test1/testdir");
  journal.added("prefix4", "path4", node, true);
  free(node);
  journal.close();

  journal.open("r");
  while (journal.getLine(&ts, &prefix, &path, &node) > 0) {
    cout << "Prefix: " << prefix << endl;
    cout << "Path:   " << path << endl;
    cout << "TS:     " << ts << endl;
    if (node == NULL) {
      cout << "Type:   removed" << endl;
    } else {
      switch (node->type()) {
        case 'f':
          cout << "Type:   file" << endl;
          break;
        case 'l':
          cout << "Type:   link" << endl;
          break;
        default:
          cout << "Type:   other" << endl;
      }
      cout << "Name:   " << node->name() << endl;
      cout << "Size:   " << node->size() << endl;
      switch (node->type()) {
        case 'f':
          cout << "Chcksm: " << ((File*) node)->checksum() << endl;
          break;
        case 'l':
          cout << "Link:   " << ((Link*) node)->link() << endl;
      }
      free(node);
    }
    cout << endl;
  }
  journal.close();
  free(line);

  return 0;
}
