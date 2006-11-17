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

/* TODO Not all functions are tested */

#include "tools.c"

int terminating(void) {
  return 0;
}

int main(void) {
  char line[256] = "";

  strcpy(line, "/this/is/a/line");
  printf("Check slashes to '%s': ", line);
  no_trailing_slash(line);
  printf("'%s'\n", line);

  strcpy(line, "/this/is/a/line/");
  printf("Check slashes to '%s': ", line);
  no_trailing_slash(line);
  printf("'%s'\n", line);

  strcpy(line, "/this/is/a/line/////////////");
  printf("Check slashes to '%s': ", line);
  no_trailing_slash(line);
  printf("'%s'\n", line);

  strcpy(line, "This is a text which I like");
  printf("Converting '%s' to lower case\n", line);
  strtolower(line);
  printf("-> gives '%s'\n", line);

  strcpy(line, "C:\\Program Files\\HBackup\\hbackup.EXE");
  printf("Converting '%s' to linux style\n", line);
  pathtolinux(line);
  printf("-> gives '%s'\n", line);
  printf("Then to lower case\n");
  strtolower(line);
  printf("-> gives '%s'\n", line);

  return 0;
}
