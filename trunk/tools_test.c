/* Herve Fache

20061025 Creation
*/

#include "tools.c"

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
