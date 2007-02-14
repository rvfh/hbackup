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

#ifndef FILE_LIST_H
#define FILE_LIST_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <sys/types.h>
/* Declare lstat and S_* macros */
#ifndef __USE_BSD
#define __USE_BSD
#endif
#include <sys/stat.h>
#include <unistd.h>

#ifndef LIST_H
#error You must include list.h before filelist.h
#endif

#ifndef FILTERS_H
#error You must include filters.h before filelist.h
#endif

#ifndef PARSERS_H
#error You must include parsers.h before filelist.h
#endif

class FileList {
  List*           _files;
  const Filters*  _filters;
  const Parsers*  _parsers;
  int             _mount_path_length;
  int             iterate_directory(
    const string&   path,
    Parser*         parser);
public:
  FileList(
    const string&   mount_path,
    const Filters*  filters,
    const Parsers*  parsers);
  ~FileList();
  List* getList();
};

#endif
