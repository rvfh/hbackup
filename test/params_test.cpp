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

#include "params.h"

int main(void) {
  char   line[256] = "";
  char   keyword[256];
  char   type[256];
  string s;

  strcpy(line, "");
  cout << "Read " << params_readline(line, keyword, type, &s)
    << " parameters from " << line << ": '" << keyword << "' '" << type
    << "' '" << s << "'" <<  endl;

  strcpy(line, "# Normal comment");
  cout << "Read " << params_readline(line, keyword, type, &s)
    << " parameters from " << line << ": '" << keyword << "' '" << type
    << "' '" << s << "'" <<  endl;

  strcpy(line, " \t# Displaced comment");
  cout << "Read " << params_readline(line, keyword, type, &s)
    << " parameters from " << line << ": '" << keyword << "' '" << type
    << "' '" << s << "'" <<  endl;

  strcpy(line, "\tkey # Comment");
  cout << "Read " << params_readline(line, keyword, type, &s)
    << " parameters from " << line << ": '" << keyword << "' '" << type
    << "' '" << s << "'" <<  endl;

  strcpy(line, "key\t \"string\" # Comment");
  cout << "Read " << params_readline(line, keyword, type, &s)
    << " parameters from " << line << ": '" << keyword << "' '" << type
    << "' '" << s << "'" <<  endl;

  strcpy(line, "key \ttype\t\"string\" # Comment");
  cout << "Read " << params_readline(line, keyword, type, &s)
    << " parameters from " << line << ": '" << keyword << "' '" << type
    << "' '" << s << "'" <<  endl;

  strcpy(line, "key\t string \t# Comment");
  cout << "Read " << params_readline(line, keyword, type, &s)
    << " parameters from " << line << ": '" << keyword << "' '" << type
    << "' '" << s << "'" <<  endl;

  strcpy(line, "key \ttype\t string # Comment");
  cout << "Read " << params_readline(line, keyword, type, &s)
    << " parameters from " << line << ": '" << keyword << "' '" << type
    << "' '" << s << "'" <<  endl;

  strcpy(line, "key \ttype \t\"string # Comment");
  cout << "Read " << params_readline(line, keyword, type, &s)
    << " parameters from " << line << ": '" << keyword << "' '" << type
    << "' '" << s << "'" <<  endl;

  strcpy(line, "key \ttype string\" # Comment");
  cout << "Read " << params_readline(line, keyword, type, &s)
    << " parameters from " << line << ": '" << keyword << "' '" << type
    << "' '" << s << "'" <<  endl;

  strcpy(line, "key\t \"this is\ta \tstring\" # Comment");
  cout << "Read " << params_readline(line, keyword, type, &s)
    << " parameters from " << line << ": '" << keyword << "' '" << type
    << "' '" << s << "'" <<  endl;

  strcpy(line, "key \ttype\t \"this is\ta \tstring\" # Comment");
  cout << "Read " << params_readline(line, keyword, type, &s)
    << " parameters from " << line << ": '" << keyword << "' '" << type
    << "' '" << s << "'" <<  endl;

  return 0;
}
