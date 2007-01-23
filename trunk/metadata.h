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

#ifndef METADATA_H
#define METADATA_H

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
#include "list.h"

/* Our data */
typedef struct {
  mode_t type;    /* type */
  time_t mtime;   /* time of last modification */
  off_t  size;    /* total size, in bytes */
  uid_t  uid;     /* user ID of owner */
  gid_t  gid;     /* group ID of owner */
  mode_t mode;    /* permissions */
} metadata_t;

extern int metadata_get(const char *path, metadata_t *metadata);

#endif
