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

#ifndef PARSER_H
#define PARSER_H

#ifndef FILES_H
#error You must include files.h before parser.h
#endif

typedef enum {
  parser_disabled,
  parser_controlled,
  parser_modified,
  parser_modifiedandothers,
  parser_others
} parser_mode_t;

class Parser {
public:
  virtual ~Parser() {};
  // Just to know the parser used
  virtual string name() = 0;
  // This will create an appropriate parser for the directory if relevant
  virtual Parser* isControlled(const string& dir_path) = 0;
  // That tells use whether to ignore the file, i.e. not back it up
  virtual bool ignore(const filedata_t *file_data) = 0;
  // For debug purposes
  virtual void list() {};
};

class IgnoreParser : public Parser {
  string name() {
    return "ignore";
  };
  // Fail on directory
  Parser* isControlled(const string& dir_path) {
    return NULL;
  };
  // Ignore all files
  bool ignore(const filedata_t *file_data) {
    return true;
  };
};

#endif
