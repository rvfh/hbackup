/* Herve Fache

20061022 Creation
*/

#include "params.c"

int main(void) {
  char line[256] = "";
  char keyword[256];
  char type[256];
  char string[256];

  strcpy(line, "/this/is/a/line");
  printf("Check slashes to '%s': ", line);
  one_trailing_slash(line);
  printf("'%s'\n", line);

  strcpy(line, "/this/is/a/line/");
  printf("Check slashes to '%s': ", line);
  one_trailing_slash(line);
  printf("'%s'\n", line);

  strcpy(line, "/this/is/a/line/////////////");
  printf("Check slashes to '%s': ", line);
  one_trailing_slash(line);
  printf("'%s'\n", line);

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
