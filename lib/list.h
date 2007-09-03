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

#ifndef LIST_H
#define LIST_H

namespace hbackup {

class DbList : public list<DbData> {
  int  load_v2(
    FILE*         readfile);
public:
  int  open(
    const string& path,
    const string& filename);
};

class List : public Stream {
  String          _line;
  // -1: error, 0: read again, 1: use current
  int             _line_status;
  int copyUntil(
    List&         list,
    StrPath&      prefix,
    StrPath&      path,
    int*          status);
public:
  List(
    const char*   dir_path,
    const char*   name = "") :
    Stream(dir_path, name) {}
  // Open file, for read or write (no append)
  int open(
    const char*   req_mode);
  // Close file
  int close();
  // Fake Loading current line from file
  ssize_t currentLine();
  // Load next line from file
  ssize_t nextLine();
  // Skip to given prefix
  bool findPrefix(const char* prefix);
  // Convert one 'line' of data (only works for journal atm)
  int getEntry(
    time_t*       timestamp,
    char**        prefix,
    char**        path,
    Node**        node);
  // Add a journal record of added file
  int added(
    const char*   prefix,
    const char*   path,
    const Node*   node,
    time_t        timestamp = -1);
  // Add a journal record of removed file
  int removed(
    const char*   prefix,
    const char*   path,
    time_t        timestamp = -1);
  // Get a list of active records for given prefix and paths
  void getList(
    const char*   prefix,
    const char*   base_path,
    const char*   rel_path,
    list<Node*>&  list);
  // Merge list and backup into this list
  int  merge(
    List&         list,
    List&         journal);
};

}

#endif
