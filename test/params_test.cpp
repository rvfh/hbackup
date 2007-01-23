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

#include "params.cpp"

int main(void) {
  char line[256] = "";
  char keyword[256];
  char type[256];
  char string[256];

  strcpy(line, "");
  printf("Read %u parameters from %s: '%s' '%s' '%s'\n",
    params_readline(line, keyword, type, string), line, keyword, type, string);

  strcpy(line, "# Normal comment");
  printf("Read %u parameters from %s: '%s' '%s' '%s'\n",
    params_readline(line, keyword, type, string), line, keyword, type, string);

  strcpy(line, " \t# Displaced comment");
  printf("Read %u parameters from %s: '%s' '%s' '%s'\n",
    params_readline(line, keyword, type, string), line, keyword, type, string);

  strcpy(line, "\tkey # Comment");
  printf("Read %u parameters from %s: '%s' '%s' '%s'\n",
    params_readline(line, keyword, type, string), line, keyword, type, string);

  strcpy(line, "key\t \"string\" # Comment");
  printf("Read %u parameters from %s: '%s' '%s' '%s'\n",
    params_readline(line, keyword, type, string), line, keyword, type, string);

  strcpy(line, "key \ttype\t \"string\" # Comment");
  printf("Read %u parameters from %s: '%s' '%s' '%s'\n",
    params_readline(line, keyword, type, string), line, keyword, type, string);

  return 0;
}
