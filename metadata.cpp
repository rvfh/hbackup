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

#include <string.h>
#include "metadata.h"

int metadata_get(const char *path, metadata_t *metadata_p) {
  struct stat metadata;

  if (lstat(path, &metadata)) {
    // errno set by lstat
    return 2;
  }
  /* Fill in file information */
  metadata_p->type  = metadata.st_mode & S_IFMT;
  metadata_p->mtime = metadata.st_mtime;
  metadata_p->size  = metadata.st_size;
  metadata_p->uid   = metadata.st_uid;
  metadata_p->gid   = metadata.st_gid;
  metadata_p->mode  = metadata.st_mode & ~S_IFMT;
  return 0;
}
