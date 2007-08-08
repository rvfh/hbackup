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

#include <fstream>
#include <iostream>
#include <string>
#include <sys/stat.h>

using namespace std;

#include "files.h"
#include "parsers.h"
#include "cvs_parser.h"

using namespace hbackup;

char* CvsParser::_control_dir = "CVS";
char* CvsParser::_entries = "Entries";

string CvsParser::name() const {
  return "CVS";
}

Parser *CvsParser::isControlled(const string& dir_path) const {
  // Parent under control, this is the control directory
  string control_dir = "/" + string(_control_dir);
  if (! _dummy
   && (dir_path.size() > control_dir.size())
   && (dir_path.substr(dir_path.size() - control_dir.size()) == control_dir)) {
    return NULL;
  }

  // If control directory exists and contains an entries file, assume control
  if (! File2((dir_path + "/" + _control_dir).c_str(), _entries).isValid()) {
    if (! _dummy) {
      cerr << "Directory should be under " << name() << " control: "
        << dir_path << endl;
      return new IgnoreParser;
    } else {
      return NULL;
    }
  } else {
    return new CvsParser(_mode, dir_path);
  }
}

CvsParser::CvsParser(parser_mode_t mode, const string& dir_path) {
  string    path = dir_path + "/" + _control_dir + "/" + _entries;
  ifstream  entries(path.c_str());

  // Save mode
  _mode = mode;

  /* Fill in list of controlled files */
  while (! entries.eof()) {
    string  buffer;
    getline(entries, buffer);
    cvs_entry_t cvs_entry;

    if (buffer.substr(0,1) == "D") {
      if (buffer.size() == 1) {
        // If a directory contains no subdirectories, the last line of the
        // entries file is a single 'D'
        continue;
      }
      cvs_entry.type = 'd';
      buffer.erase(0,1);
    } else {
      cvs_entry.type = 'f';
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

bool CvsParser::ignore(const File& file_data) {
  // No need to check more
  if (_mode == parser_modifiedandothers) {
    return false;
  }

  // Do not ignore control directory
  if ((file_data.name() == _control_dir) && (file_data.type() == 'd')) {
    return false;
  }

  // Look for match in list
  bool controlled = false;
  vector<cvs_entry_t>::iterator i;
  for (i = _files.begin(); i != _files.end(); i++) {
    if ((i->name == file_data.name()) && (i->type == file_data.type())) {
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
