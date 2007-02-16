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

#include <iostream>
#include <vector>

#include "files.h"
#include "parser.h"
#include "parsers.h"

class TestParser : public Parser {
  parser_mode_t _mode;
  int           _index;
public:
  // Constructor for parsers list
  TestParser(parser_mode_t mode) : _mode(mode) {
    _index = 1;
    cout << "Contructing for mode: " << _mode << endl;
  }
  // Constructor for directory parsing
  TestParser(parser_mode_t mode, const string& dir_path) {
    _mode = mode;
    _index = 2;
    cout << "Contructing for mode: " << mode
      << " and path: " << dir_path << endl;
  }
  ~TestParser() {
    cout << "Destroying (index " << _index << ")" << endl;
  }
  // Just to know the parser used
  string name() {
    return "test";
  }
  // This will create an appropriate parser for the directory if relevant
  Parser* isControlled(const string& dir_path) {
    return new TestParser(_mode, dir_path);
  }
  // That tells use whether to ignore the file, i.e. not back it up
  bool ignore(const filedata_t *file_data) {
    return false;
  }
  // For debug purposes
  void list() {
    cout << "Displaying list" << endl;
  }
};

int main(void) {
  Parsers*  parsers;
  Parser*   parser;

  parsers = new Parsers;
  parsers->push_back(new TestParser(parser_modified));
  parsers->push_back(new IgnoreParser());
  parser = parsers->isControlled("test");
  if (parser != NULL) {
    parser->list();
    delete parser;
  }
  parsers->list();
  delete parsers;

  return 0;
}
