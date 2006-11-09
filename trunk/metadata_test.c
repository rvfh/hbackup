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

#include "metadata.c"

int main(void) {
  metadata_t metadata;

  printf("metadata_get\n");
  metadata_get("test/testfile", &metadata);
  printf(" * type: 0x%08x\n", metadata.type);
  printf(" * size: %u\n", (unsigned int) metadata.size);
  printf(" * mtime: %u\n", (unsigned int) metadata.mtime);
  printf(" * uid: %u\n", metadata.uid);
  printf(" * gid: %u\n", metadata.gid);
  printf(" * mode: 0x%08x\n", metadata.mode);

  return 0;
}
