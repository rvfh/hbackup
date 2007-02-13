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

  // Create pseudo parsers list member
  parser_list = new CvsParser(parser_controlled);

  /* Directory */
  if ((parser = parser_list->isControlled("test/")) == NULL) {
    printf("test is not under CVS control\n");
  } else {
    parser->list();
    delete parser;
  }

  /* Directory */
  if ((parser = parser_list->isControlled("test/cvs")) == NULL) {
    printf("test/cvs is not under CVS control\n");
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
    delete parser;
  }

  /* Directory */
  if ((parser = parser_list->isControlled("test/cvs/dirutd")) == NULL) {
    printf("test/cvs/dirutd is not under CVS control\n");
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
      printf("test/cvs/dirutd/fileoth is not under CVS control\n");
    }
    delete parser;
  }

  /* Directory */
  if ((parser = parser_list->isControlled("test/cvs/dirbad")) == NULL) {
    printf("test/cvs/dirbad is not under CVS control\n");
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

  return 0;
}
