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

#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include "tools.h"
#include "metadata.h"
#include "common.h"
#include "parser.h"
#include "cvs_parser.h"

CvsParser::CvsParser(parser_mode_t mode, const string& dir_path) {
  string    path = dir_path + "/CVS/Entries";
  ifstream  entries(path.c_str());

  /* Fill in list of controlled files */
  while (! entries.eof()) {
    string  buffer;
    getline(entries, buffer);
    cvs_entry_t cvs_entry;

    if (buffer.substr(0,1) == "D") {
      cvs_entry.type = S_IFDIR;
      buffer.erase(0,1);
    } else {
      cvs_entry.type = S_IFREG;
    }
    if (buffer.substr(0,1) != "/") {
      continue;
    }
    buffer.erase(0,1);
    unsigned int pos = buffer.find("/");
    if (pos == string::npos) {
      continue;
    }
    buffer.erase(pos);
    cvs_entry.name = buffer;
    _files.push_back(cvs_entry);
  }
  /* Close file */
  entries.close();
}

string CvsParser::name() {
  return "cvs";
}

Parser *CvsParser::isControlled(const string& dir_path) {
  // If CVS dir and entries file exist, assume CVS control
  if (testfile(dir_path + "/CVS/Entries", false)) {
    return NULL;
  } else {
    return new CvsParser(_mode, dir_path);
  }
}

bool CvsParser::ignore(const filedata_t *file_data) {
  // Get file base name
  string file_name;
  unsigned int pos = file_data->path.rfind("/");
  if (pos == string::npos) {
    file_name = file_data->path;
  } else {
    file_name = file_data->path.substr(pos + 1);
  }

  // Look for match in list
  for (unsigned int i = 0; i < _files.size(); i++) {
    if ((_files[i].name == file_name)
     && (_files[i].type == file_data->metadata.type)){
      return false;
    }
  }
  return true;

}
