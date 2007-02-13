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

#ifndef PARSERS_H
#define PARSERS_H

#ifndef PARSER_H
#error You must include parser.h before parsers.h
#endif

class Parsers : public vector<Parser *> {
public:
  ~Parsers() {
    for (unsigned int i = 0; i < this->size(); i++) {
      delete (*this)[i];
    }
  }
  Parser* isControlled(const string& dir_path) const {
    Parser *parser;
    for (unsigned int i = 0; i < this->size(); i++) {
      parser = (*this)[i]->isControlled(dir_path);
      if (parser != NULL) {
        return parser;
      }
    }
    return NULL;
  }
  void list() {
    cout << "List: " << this->size() << " parser(s)" << endl;
    for (unsigned int i = 0; i < this->size(); i++) {
      cout << "-> " << (*this)[i]->name() << endl;
    }
  }
};

#endif
