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
#include "metadata.h"
#include "common.h"
#include "parser.h"
#include "cvs_parser.h"

int terminating(void) {
  return 0;
}

int main(void) {
  filedata_t  file_data;
  Parser*     parser_list;
  Parser*     parser;
  Parser*     parser2;

  // Create pseudo parsers list member
  parser_list = new CvsParser(parser_controlled);

  /* Directory */
  file_data.path = "test";
  if ((parser = parser_list->isControlled(file_data.path)) == NULL) {
    cout << file_data.path << " is not under CVS control" << endl;
  } else {
    parser->list();
    delete parser;
  }

  /* Directory */
  file_data.path = "test/cvs";
  if ((parser = parser_list->isControlled(file_data.path)) == NULL) {
    cout << file_data.path << " is not under CVS control" << endl;
  } else {
    parser->list();
    /* Files */
    file_data.path = "test/cvs/nofile";
    file_data.metadata.type = S_IFREG;
    if (parser->ignore(&file_data)) {
      cout << file_data.path << " is not under CVS control" << endl;
    }
    file_data.path = "test/cvs/filenew.c";
    file_data.metadata.type = S_IFREG;
    if (parser->ignore(&file_data)) {
      cout << file_data.path << " is not under CVS control" << endl;
    }
    file_data.path = "test/cvs/filemod.o";
    file_data.metadata.type = S_IFREG;
    if (parser->ignore(&file_data)) {
      cout << file_data.path << " is not under CVS control" << endl;
    }
    file_data.path = "test/cvs/fileutd.h";
    file_data.metadata.type = S_IFREG;
    if (parser->ignore(&file_data)) {
      cout << file_data.path << " is not under CVS control" << endl;
    }
    file_data.path = "test/cvs/fileoth";
    file_data.metadata.type = S_IFREG;
    if (parser->ignore(&file_data)) {
      cout << file_data.path << " is not under CVS control" << endl;
    }
    file_data.path = "test/cvs/dirutd";
    file_data.metadata.type = S_IFDIR;
    if (parser->ignore(&file_data)) {
      cout << file_data.path << " is not under CVS control" << endl;
    }
    file_data.path = "test/cvs/diroth";
    file_data.metadata.type = S_IFDIR;
    if (parser->ignore(&file_data)) {
      cout << file_data.path << " is not under CVS control" << endl;
    }
    file_data.path = "test/cvs/CVS";
    file_data.metadata.type = S_IFDIR;
    if (parser->ignore(&file_data)) {
      cout << file_data.path << " is not under CVS control" << endl;
    }
    file_data.path = "test/cvs/CVS";
    if ((parser2 = parser->isControlled(file_data.path)) == NULL) {
      cout << file_data.path << " is not under CVS control" << endl;
    } else {
      delete parser2;
    }
    file_data.path = "test/cvs/diroth";
    if ((parser2 = parser->isControlled(file_data.path)) == NULL) {
      cout << file_data.path << " is not under CVS control" << endl;
    } else {
      delete parser2;
    }
    delete parser;
  }

  /* Directory */
  file_data.path = "test/cvs/dirutd";
  if ((parser = parser_list->isControlled(file_data.path)) == NULL) {
    cout << file_data.path << " is not under CVS control" << endl;
  } else {
    parser->list();
    /* Files */
    file_data.path = "test/cvs/dirutd/fileutd";
    file_data.metadata.type = S_IFREG;
    if (parser->ignore(&file_data)) {
      cout << file_data.path << " is not under CVS control" << endl;
    }
    file_data.path = "test/cvs/dirutd/fileoth";
    file_data.metadata.type = S_IFREG;
    if (parser->ignore(&file_data)) {
      cout << file_data.path << " is not under CVS control" << endl;
    }
    delete parser;
  }

  /* Directory */
  file_data.path = "test/cvs/dirbad";
  if ((parser = parser_list->isControlled(file_data.path)) == NULL) {
    cout << file_data.path << " is not under CVS control" << endl;
  } else {
    parser->list();
    /* Files */
    file_data.path = "test/cvs/dirbad/fileutd";
    file_data.metadata.type = S_IFREG;
    if (parser->ignore(&file_data)) {
      cout << file_data.path << " is not under CVS control" << endl;
    }
    file_data.path = "test/cvs/dirbad/fileoth";
    file_data.metadata.type = S_IFREG;
    if (parser->ignore(&file_data)) {
      cout << file_data.path << " is not under CVS control" << endl;
    }
    delete parser;
  }

  /* CVS directory */
  file_data.path = "test/cvs/CVS";
  if ((parser = parser_list->isControlled(file_data.path)) == NULL) {
    cout << file_data.path << " is not under CVS control" << endl;
  } else {
    parser->list();
    delete parser;
  }

  return 0;
}
