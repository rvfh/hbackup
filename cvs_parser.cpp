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

// TODO: this is incomplete:
//  * D on its own (_all_files)
//  * other nice traps?

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

  // Save mode
  _mode = mode;
  _dummy = false;

  /* Fill in list of controlled files */
  _all_files = false;
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
  // Parent under CVS control, this is the control directory
  if (! _dummy
   && (dir_path.size() > 4)
   && (dir_path.substr(dir_path.size() - 4) == "/CVS")) {
    return NULL;
  }

  // If CVS dir and entries file exist, assume CVS control
  if (testfile(dir_path + "/CVS/Entries", false)) {
    if (! _dummy) {
      cerr << "Directory should be under CVS control: " << dir_path << endl;
      return new IgnoreParser;
    } else {
      return NULL;
    }
  } else {
    return new CvsParser(_mode, dir_path);
  }
}

bool CvsParser::ignore(const filedata_t *file_data) {
  // Deal with no-work cases
  if (  (_all_files)
     // We don't know whether controlled files are modified or not
     || (_mode == parser_modifiedandothers)
     || (_mode == parser_disabled)) {
    return false;
  }

  // Get file base name
  string file_name;
  unsigned int pos = file_data->path.rfind("/");
  if (pos == string::npos) {
    file_name = file_data->path;
  } else {
    file_name = file_data->path.substr(pos + 1);
  }

  // Do not ignore CVS directory
  if (  (file_name == "CVS")
     && (file_data->metadata.type == S_IFDIR)) {
    return false;
  }

  // Look for match in list
  bool controlled = false;
  for (unsigned int i = 0; i < _files.size(); i++) {
    if ((_files[i].name == file_name)
     && (_files[i].type == file_data->metadata.type)) {
      controlled = true;
      break;
    }
  }

  // Deal with result
  switch (_mode) {
    // We don't know whether controlled files are modified or not
    case parser_controlled:
    case parser_modified:
      if (controlled) return false; else return true;
    case parser_modifiedandothers:
      return false;
    case parser_others:
      if (controlled) return true; else return false;
    default:  // parser_disabled
      return false;
  }
}
