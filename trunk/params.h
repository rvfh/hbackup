/* Herve Fache

20061022 Creation
*/

#ifndef PARAMS_H
#define PARAMS_H

/* Convert string to lower case */
extern void strtolower(char *string);

/* Convert path to UNIX style */
extern void pathtolinux(char *path);

/* Read parameters from line */
extern int params_readline(const char *line, char *keyword, char *type,
  char *string);

#endif
