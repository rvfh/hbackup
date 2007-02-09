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
#include <string>

#include "params.h"

int params_readline(const char *linein, char *keyword, char *type,
    string *s) {
  string        line(linein);
  unsigned int  pos;
  unsigned int  minpos;
  int           param_count = 0;

  // Reset all
  strcpy(keyword, "");
  strcpy(type, "");
  *s = "";

  // Get rid of comments
  pos = line.find("#");
  if (pos != string::npos) {
    line.erase(pos);
  }

  // Remove leading blanks
  pos = 0;
  while ((pos < line.size()) && ((line[pos] == ' ') || (line[pos] == '\t'))) {
    pos++;
  }
  line.erase(0, pos);

  // Make sure there is something left to deal with
  if (line.size() == 0) {
    return param_count;
  }

  // Remove trailing blanks
  pos = line.size() - 1;
  while ((pos > 0) && ((line[pos] == ' ') || (line[pos] == '\t'))) {
    pos--;
  }
  if (pos < (line.size() - 1)) {
    line.erase(pos + 1);
  }

  // Make sure there is something left to deal with
  if (line.size() == 0) {
    return 0;
  }

  // Find next blank
  minpos = line.find(" ");
  pos = line.find("\t");
  if (pos == string::npos) {
    if (minpos == string::npos) {
      pos = line.size();
    } else {
      pos = minpos;
    }
  } else
  if ((minpos != string::npos) && (pos > minpos)) {
    pos = minpos;
  }

  // Extract keyword
  string arg1 = line.substr(0, pos);
  strcpy(keyword, arg1.c_str());
  param_count++;

  // Remove keyword and all leading blanks
  pos++;
  while ((pos < line.size()) && ((line[pos] == ' ') || (line[pos] == '\t'))) {
    pos++;
  }
  line.erase(0, pos);

  // Make sure there is something left to deal with
  if (line.size() == 0) {
    return param_count;
  }

  // Find last blank or double quote
  if (line[line.size() - 1] == '\"') {
    line.erase(line.size() - 1, 1);
    pos = line.rfind("\"");
    if (pos == string::npos) {
      return -1;
    } else {
      line.erase(pos, 1);
    }
  } else {
    minpos = line.rfind(" ");
    pos = line.rfind("\t");
    if (pos == string::npos) {
      if (minpos == string::npos) {
        pos = 0;
      } else {
        pos = minpos + 1;
      }
    } else {
      if ((minpos != string::npos) && (pos < minpos)) {
        pos = minpos + 1;
      } else {
        pos++;
      }
    }
  }

  // Extract and remove string
  *s = line.substr(pos);

  // Check string
  if ((*s)[0] == '\"') {
    return -1;
  }

  // Remove string
  line.erase(pos);
  param_count++;

  // Remove trailing blanks
  pos = line.size() - 1;
  while ((pos > 0) && ((line[pos] == ' ') || (line[pos] == '\t'))) {
    pos--;
  }
  if (pos < (line.size() - 1)) {
    line.erase(pos + 1);
  }

  // Extract type
  if (line.size() != 0) {
    strcpy(type, line.c_str());
    param_count++;
  }

  return param_count;
}
