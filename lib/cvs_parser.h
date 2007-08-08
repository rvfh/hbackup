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

#ifndef CVS_PARSER_H
#define CVS_PARSER_H

// List does not work
#include <vector>

using namespace std;

#include "files.h"
#include "parsers.h"

namespace hbackup {

class CvsParser : public Parser {
  struct cvs_entry_t {
    string  name;   /* File name */
    char    type;   /* File type */
  };

  static char*      _control_dir;
  static char*      _entries;
  vector<cvs_entry_t> _files;
public:
  // Constructor for parsers list
  CvsParser(parser_mode_t mode) : Parser(mode) {}
  // Constructor for directory parsing
  CvsParser(parser_mode_t mode, const string& dir_path);
  // Just to know the parser used
  string name() const;
  // This will create an appropriate parser for the directory if relevant
  Parser* isControlled(const string& dir_path) const;
  // That tells use whether to ignore the file, i.e. not back it up
  bool ignore(const File& file_data);
  // For debug purposes
  void list() {
    cout << "List: " << _files.size() << " file(s)" << endl;
    vector<cvs_entry_t>::iterator i;
    for (i = _files.begin(); i != _files.end(); i++) {
      cout << "-> " << i->name << " (" << i->type << ")" << endl;
    }
  }
};

}

#endif
