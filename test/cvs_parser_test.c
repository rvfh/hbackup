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

/* Yes, include C file */
#include "cvs_parser.c"

static char *cvs_show(const void *payload) {
  char *string = NULL;

  asprintf(&string, "%s", ((cvs_entry_t *) payload)->name);
  return string;
}

int main(void) {
  void *handle;
  void *handle1;
  void *handle2;
  void *handle3;
  void *handle4;
  filedata_t file_data;
  list_t *parsers_handle = NULL;
  parser_t *cvs_parser;

  /* Need to give this some room */
  file_data.path = malloc(256);

  /* Creation */
  cvs_parser_new();

  /* Directory */
  if (cvs_dir_check(&handle1, "test/") == parser_dir_other) {
    printf("test is not under CVS control\n");
  } else {
    list_show(handle1, NULL, cvs_show);
    cvs_dir_leave(handle1);
  }

  /* Directory */
  if (cvs_dir_check(&handle2, "test/cvs") == parser_dir_other) {
    printf("test/cvs is not under CVS control\n");
  } else {
    list_show(handle2, NULL, cvs_show);
    /* Files */
    strcpy(file_data.path, "test/cvs/nofile");
    if (cvs_file_check(handle2, &file_data) == parser_file_other) {
      printf("%s is not under CVS control\n", file_data.path);
    }
    strcpy(file_data.path, "test/cvs/filenew.c");
    if (cvs_file_check(handle2, &file_data) == parser_file_other) {
      printf("%s is not under CVS control\n", file_data.path);
    }
    strcpy(file_data.path, "test/cvs/filemod.o");
    if (cvs_file_check(handle2, &file_data) == parser_file_other) {
      printf("%s is not under CVS control\n", file_data.path);
    }
    strcpy(file_data.path, "test/cvs/fileutd.h");
    if (cvs_file_check(handle2, &file_data) == parser_file_other) {
      printf("%s is not under CVS control\n", file_data.path);
    }
    strcpy(file_data.path, "test/cvs/fileoth");
    if (cvs_file_check(handle2, &file_data) == parser_file_other) {
      printf("%s is not under CVS control\n", file_data.path);
    }
    strcpy(file_data.path, "test/cvs/dirutd");
    if (cvs_file_check(handle2, &file_data) == parser_file_other) {
      printf("%s is not under CVS control\n", file_data.path);
    }
    strcpy(file_data.path, "test/cvs/diroth");
    if (cvs_file_check(handle2, &file_data) == parser_file_other) {
      printf("%s is not under CVS control\n", file_data.path);
    }
    cvs_dir_leave(handle2);
  }

  if (cvs_dir_check(&handle3, "test/cvs/dirutd") == parser_dir_other) {
    printf("test/cvs/dirutd is not under CVS control\n");
  } else {
    list_show(handle3, NULL, cvs_show);
    strcpy(file_data.path, "test/cvs/dirutd/fileutd");
    if (cvs_file_check(handle3, &file_data) == parser_file_other) {
      printf("%s is not under CVS control\n", file_data.path);
    }
    strcpy(file_data.path, "test/cvs/dirutd/fileoth");
    if (cvs_file_check(handle3, &file_data) == parser_file_other) {
      printf("test/cvs/dirutd/fileoth is not under CVS control\n");
    }
    cvs_dir_leave(handle3);
  }

  if (cvs_dir_check(&handle4, "test/cvs/dirbad") == parser_dir_other) {
    printf("test/cvs/dirbad is not under CVS control\n");
  } else {
    list_show(handle4, NULL, cvs_show);
    strcpy(file_data.path, "test/cvs/dirbad/fileutd");
    if (cvs_file_check(handle4, &file_data) == parser_file_other) {
      printf("%s is not under CVS control\n", file_data.path);
    }
    strcpy(file_data.path, "test/cvs/dirbad/fileoth");
    if (cvs_file_check(handle4, &file_data) == parser_file_other) {
      printf("%s is not under CVS control\n", file_data.path);
    }
    cvs_dir_leave(handle4);
  }

  cvs_file_check(NULL, &file_data);
  cvs_dir_leave(NULL);

  parsers_new(&parsers_handle);
  parsers_add(parsers_handle, parser_controlled, cvs_parser_new());
  cvs_parser = list_entry_payload(list_next(parsers_handle, NULL));

  if (cvs_parser->dir_check(&handle, "test/cvs") == parser_dir_other) {
    printf("test/cvs is not under CVS control\n");
  } else {
    list_show(handle, NULL, cvs_show);
    strcpy(file_data.path, "test/cvs/fileutd");
    if (cvs_parser->file_check(handle, &file_data) == parser_file_other) {
      printf("%s is not under CVS control\n", file_data.path);
    }
    strcpy(file_data.path, "test/cvs/fileoth");
    if (cvs_parser->file_check(handle, &file_data) == parser_file_other) {
      printf("%s is not under CVS control\n", file_data.path);
    }
    strcpy(file_data.path, "test/cvs/dirutd");
    if (cvs_parser->file_check(handle, &file_data) == parser_file_other) {
      printf("%s is not under CVS control\n", file_data.path);
    }
    strcpy(file_data.path, "test/cvs/diroth");
    if (cvs_parser->file_check(handle, &file_data) == parser_file_other) {
      printf("%s is not under CVS control\n", file_data.path);
    }
    cvs_parser->dir_leave(handle);
  }
  parsers_free(parsers_handle);
  free(file_data.path);

  return 0;
}
