/* Herve Fache

20061025 Creation
*/

#ifndef TOOLS_H
#define TOOLS_H

/* Make sure string finish by one slash and only one */
extern void one_trailing_slash(char *string);

/* Convert string to lower case */
extern void strtolower(char *string);

/* Convert path to UNIX style */
extern void pathtolinux(char *path);

/* Test whether dir exists, create it if requested */
extern int testdir(const char *path, int create);

/* Test whether file exists, create it if requested */
extern int testfile(const char *path, int create);

#endif
