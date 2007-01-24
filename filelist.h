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
#include "metadata.h"
#include "filters.h"

/* Create list of files from path, using filters */
extern int filelist_new(
  const char    *path,
  const Filter  *filter,
  const List    *parsers);

/* Destroy list of files */
extern void filelist_free(void);

/* Obtain list of files */
extern List *filelist_get(void);

#endif
