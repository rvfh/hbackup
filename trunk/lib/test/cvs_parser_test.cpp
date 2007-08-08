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

using namespace std;

#include <iostream>
#include <vector>
#include <list>
#include <sys/stat.h>

#include "files.h"
#include "parsers.h"
#include "cvs_parser.h"

using namespace hbackup;

int verbosity(void) {
  return 0;
}

int terminating(void) {
  return 0;
}

int main(void) {
  File        *file_data;
  Parser*     parser_list;
  Parser*     parser;
  Parser*     parser2;

  // Create pseudo parsers list member
  parser_list = new CvsParser(parser_controlled);

  /* Directory */
  file_data = new File("test1", "", 'd', 0, 0, 0, 0, 0);
  if ((parser = parser_list->isControlled(file_data->path())) == NULL) {
    cout << file_data->path() << " is not under CVS control" << endl;
  } else {
    parser->list();
    delete parser;
  }
  delete file_data;

  /* Directory */
  file_data = new File("test1/cvs", "", 'd', 0, 0, 0, 0, 0);
  if ((parser = parser_list->isControlled(file_data->path())) == NULL) {
    cout << file_data->path() << " is not under CVS control" << endl;
  } else {
    parser->list();
    /* Files */
    file_data = new File("test1/cvs/nofile", "", 'f', 0, 0, 0, 0, 0);
    if (parser->ignore(*file_data)) {
      cout << file_data->path() << " is not under CVS control" << endl;
    }
    delete file_data;
    file_data = new File("test1/cvs/filenew.c", "", 'f', 0, 0, 0, 0, 0);
    if (parser->ignore(*file_data)) {
      cout << file_data->path() << " is not under CVS control" << endl;
    }
    delete file_data;
    file_data = new File("test1/cvs/filemod.o", "", 'f', 0, 0, 0, 0, 0);
    if (parser->ignore(*file_data)) {
      cout << file_data->path() << " is not under CVS control" << endl;
    }
    delete file_data;
    file_data = new File("test1/cvs/fileutd.h", "", 'f', 0, 0, 0, 0, 0);
    if (parser->ignore(*file_data)) {
      cout << file_data->path() << " is not under CVS control" << endl;
    }
    delete file_data;
    file_data = new File("test1/cvs/fileoth", "", 'f', 0, 0, 0, 0, 0);
    if (parser->ignore(*file_data)) {
      cout << file_data->path() << " is not under CVS control" << endl;
    }
    delete file_data;
    file_data = new File("test1/cvs/dirutd", "", 'd', 0, 0, 0, 0, 0);
    if (parser->ignore(*file_data)) {
      cout << file_data->path() << " is not under CVS control" << endl;
    }
    delete file_data;
    file_data = new File("test1/cvs/diroth", "", 'd', 0, 0, 0, 0, 0);
    if (parser->ignore(*file_data)) {
      cout << file_data->path() << " is not under CVS control" << endl;
    }
    delete file_data;
    file_data = new File("test1/cvs/CVS", "", 'd', 0, 0, 0, 0, 0);
    if (parser->ignore(*file_data)) {
      cout << file_data->path() << " is not under CVS control" << endl;
    }
    delete file_data;
    file_data = new File("test1/cvs/CVS", "", 'd', 0, 0, 0, 0, 0);
    if ((parser2 = parser->isControlled(file_data->path())) == NULL) {
      cout << file_data->path() << " is not under CVS control" << endl;
    } else {
      delete parser2;
    }
    delete file_data;
    file_data = new File("test1/cvs/diroth", "", 'd', 0, 0, 0, 0, 0);
    // Use parser so directory appears as child of controlled directory
    if ((parser2 = parser->isControlled(file_data->path())) == NULL) {
      cout << file_data->path() << " is not under CVS control" << endl;
    } else {
      delete parser2;
    }
    delete file_data;
    delete parser;
  }

  /* Directory */
  file_data = new File("test1/cvs/dirutd", "", 'd', 0, 0, 0, 0, 0);
  if ((parser = parser_list->isControlled(file_data->path())) == NULL) {
    cout << file_data->path() << " is not under CVS control" << endl;
  } else {
    parser->list();
    /* Files */
    delete file_data;
    file_data = new File("test1/cvs/dirutd/fileutd", "", 'f', 0, 0, 0, 0, 0);
    if (parser->ignore(*file_data)) {
      cout << file_data->path() << " is not under CVS control" << endl;
    }
    delete file_data;
    file_data = new File("test1/cvs/dirutd/fileoth", "", 'f', 0, 0, 0, 0, 0);
    if (parser->ignore(*file_data)) {
      cout << file_data->path() << " is not under CVS control" << endl;
    }
    delete parser;
  }
  delete file_data;

  /* Directory */
  file_data = new File("test1/cvs/dirbad", "", 'd', 0, 0, 0, 0, 0);
  if ((parser = parser_list->isControlled(file_data->path())) == NULL) {
    cout << file_data->path() << " is not under CVS control" << endl;
  } else {
    parser->list();
    /* Files */
    delete file_data;
    file_data = new File("test1/cvs/dirbad/fileutd", "", 'f', 0, 0, 0, 0, 0);
    if (parser->ignore(*file_data)) {
      cout << file_data->path() << " is not under CVS control" << endl;
    }
    delete file_data;
    file_data = new File("test1/cvs/dirbad/fileoth", "", 'f', 0, 0, 0, 0, 0);
    if (parser->ignore(*file_data)) {
      cout << file_data->path() << " is not under CVS control" << endl;
    }
    delete parser;
  }
  delete file_data;

  /* CVS directory */
  file_data = new File("test1/cvs/CVS", "", 'd', 0, 0, 0, 0, 0);
  if ((parser = parser_list->isControlled(file_data->path())) == NULL) {
    cout << file_data->path() << " is not under CVS control" << endl;
  } else {
    parser->list();
    delete parser;
  }
  delete file_data;

  return 0;
}