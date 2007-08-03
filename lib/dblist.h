/*
     Copyright (C) 2007  Herve Fache

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

#ifndef DBLIST_H
#define DBLIST_H

#include "dbdata.h"
#include "list.h"

namespace hbackup {

class DbList : public SortedList<DbData> {
  bool _open;
public:
  DbList() : _open(false) {}
  bool isOpen() { return _open; }
  // Load list, skipping offset elements
  int  load(
    const string& path,
    const string& filename,
    unsigned int  offset = 0);
  // Save list, creating a backup of the old one first
  int  save(
    const string& path,
    const string& filename,
    bool          backup = false);
  int  save_journal(
    const string& path,
    const string& filename,
    unsigned int  offset = 0);
  int  open(
    const string& path,
    const string& filename);
  int  close(
    const string& path,
    const string& filename);
};

}

#endif
