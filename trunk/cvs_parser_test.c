/* Herve Fache

20061005 Creation
*/

/* Yes, include C file */
#include "cvs_parser.c"

static void cvs_show(const void *payload, char *string) {
/*  sprintf(string, "%s 0x%08x %s", ((cvs_entry_t *) payload_p)->path, ((cvs_entry_t *) payload_p)->type, ((cvs_entry_t *) payload_p)->mtime);*/
  sprintf(string, "%s", ((cvs_entry_t *) payload)->name);
}

int main(void) {
  void *handle;
  void *handle1;
  void *handle2;
  void *handle3;
  void *handle4;
  filelist_data_t file_data;
  void *parsers_handle = NULL;
  parser_t *cvs_parser;

  /* Creation */
  cvs_parser_new();

  /* Directory */
  if (cvs_dir_check(&handle1, "test/")) {
    printf("test is not under CVS control\n");
  } else {
    list_show(handle1, NULL, cvs_show);
    cvs_dir_leave(handle1);
  }

  /* Directory */
  if (cvs_dir_check(&handle2, "test/cvs")) {
    printf("test/cvs is not under CVS control\n");
  } else {
    list_show(handle2, NULL, cvs_show);
    /* Files */
    strcpy(file_data.path, "test/cvs/nofile");
    if (cvs_file_check(handle2, &file_data)) {
      printf("%s is not under CVS control\n", file_data.path);
    }
    strcpy(file_data.path, "test/cvs/filenew.c");
    if (cvs_file_check(handle2, &file_data)) {
      printf("%s is not under CVS control\n", file_data.path);
    }
    strcpy(file_data.path, "test/cvs/filemod.o");
    if (cvs_file_check(handle2, &file_data)) {
      printf("%s is not under CVS control\n", file_data.path);
    }
    strcpy(file_data.path, "test/cvs/fileutd.h");
    if (cvs_file_check(handle2, &file_data)) {
      printf("%s is not under CVS control\n", file_data.path);
    }
    strcpy(file_data.path, "test/cvs/fileoth");
    if (cvs_file_check(handle2, &file_data)) {
      printf("%s is not under CVS control\n", file_data.path);
    }
    strcpy(file_data.path, "test/cvs/dirutd");
    if (cvs_file_check(handle2, &file_data)) {
      printf("%s is not under CVS control\n", file_data.path);
    }
    strcpy(file_data.path, "test/cvs/diroth");
    if (cvs_file_check(handle2, &file_data)) {
      printf("%s is not under CVS control\n", file_data.path);
    }
    cvs_dir_leave(handle2);
  }

  if (cvs_dir_check(&handle3, "test/cvs/dirutd")) {
    printf("test/cvs/dirutd is not under CVS control\n");
  } else {
    list_show(handle3, NULL, cvs_show);
    strcpy(file_data.path, "test/cvs/dirutd/fileutd");
    if (cvs_file_check(handle3, &file_data)) {
      printf("%s is not under CVS control\n", file_data.path);
    }
    strcpy(file_data.path, "test/cvs/dirutd/fileoth");
    if (cvs_file_check(handle3, &file_data)) {
      printf("test/cvs/dirutd/fileoth is not under CVS control\n");
    }
    cvs_dir_leave(handle3);
  }

  if (cvs_dir_check(&handle4, "test/cvs/dirbad")) {
    printf("test/cvs/dirbad is not under CVS control\n");
  } else {
    list_show(handle4, NULL, cvs_show);
    strcpy(file_data.path, "test/cvs/dirbad/fileutd");
    if (cvs_file_check(handle4, &file_data)) {
      printf("%s is not under CVS control\n", file_data.path);
    }
    strcpy(file_data.path, "test/cvs/dirbad/fileoth");
    if (cvs_file_check(handle4, &file_data)) {
      printf("%s is not under CVS control\n", file_data.path);
    }
    cvs_dir_leave(handle4);
  }

  cvs_file_check(NULL, &file_data);
  cvs_dir_leave(NULL);

  parsers_new();
  parsers_add(cvs_parser_new());
  cvs_parser = parsers_next(&parsers_handle);

  if (cvs_parser->dir_check(&handle, "test/cvs")) {
    printf("test/cvs is not under CVS control\n");
  } else {
    list_show(handle, NULL, cvs_show);
    strcpy(file_data.path, "test/cvs/fileutd");
    if (cvs_parser->file_check(handle, &file_data)) {
      printf("%s is not under CVS control\n", file_data.path);
    }
    strcpy(file_data.path, "test/cvs/fileoth");
    if (cvs_parser->file_check(handle, &file_data)) {
      printf("%s is not under CVS control\n", file_data.path);
    }
    strcpy(file_data.path, "test/cvs/dirutd");
    if (cvs_parser->file_check(handle, &file_data)) {
      printf("%s is not under CVS control\n", file_data.path);
    }
    strcpy(file_data.path, "test/cvs/diroth");
    if (cvs_parser->file_check(handle, &file_data)) {
      printf("%s is not under CVS control\n", file_data.path);
    }
    cvs_parser->dir_leave(handle);
  }
  parsers_free();

  return 0;
}
