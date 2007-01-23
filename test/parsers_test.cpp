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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <iostream>
using namespace std;
#include "parsers.cpp"

static parser_t parser = {
  (parsers_dir_check_f *) 0x12345678,
  (parsers_dir_leave_f *) 0xDEADBEEF,
  (parsers_file_check_f *) 0x34567890,
  parser_disabled,
  "test parser" };

/* Use payload as argument name, cast once and for all */
static char *parsers_show(const void *payload) {
  const parser_t *parser = (const parser_t *) (payload);
  char *string = NULL;

  asprintf(&string, "%s [0x%08x, 0x%08x, 0x%08x]",
    parser->name,
    (unsigned int) parser->dir_check,
    (unsigned int) parser->dir_leave,
    (unsigned int) parser->file_check);
  return string;
}

/* TODO parsers_dir_check and parsers_file_check test */
int main(void) {
  parser_t *parser_p = new parser_t;
  List *handle1 = NULL;
  List *handle2 = NULL;

  if (parsers_new(&handle1)) {
    cout << "Failed to create\n";
  }
  *parser_p = parser;
  if (parsers_add(handle1, parser_controlled, parser_p)) {
    cout << "Failed to add\n";
  }
  handle1->show(NULL, parsers_show);
  if (parser_p != list_entry_payload(handle1->next(NULL))) {
    cout << "Parsers differ\n";
  }
  if (parsers_new(&handle2)) {
    cout << "Failed to create\n";
  }
  handle2->show(NULL, parsers_show);
  handle1->show(NULL, parsers_show);
  parsers_free(handle2);
  parsers_free(handle1);
  return 0;
}
