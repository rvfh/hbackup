/* Herve Fache

20061022 Creation
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "params.h"

static void remove_starting_blanks(char **string) {
  while (((*string)[0] == ' ') || ((*string)[0] == '\t')) {
    (*string)++;
  }
}

static void remove_trailing_blanks(char *string) {
  char *last = &string[strlen(string) - 1];

  while ((last >= string)
      && ((*last == ' ') || (*last == '\t')
       || (*last == '\n') || (*last == '\r'))) {
    *last-- = '\0';
  }
}

static char *find_blank(const char *string) {
  char *space = strchr(string, ' ');
  char *tab = strchr(string, '\t');

  if (space == NULL) {
    return tab;
  } else if (tab == NULL) {
    return space;
  } else if (space < tab) {
    return space;
  } else {
    return tab;
  }
}

int params_readline(const char *line, char *keyword, char *type,
    char *string) {
  char *linecopy = NULL;
  char *start;
  char *delim;
  int param_count = 0;

  /* Reset all */
  asprintf(&linecopy, line);
  start = linecopy;
  strcpy(keyword, "");
  strcpy(type, "");
  strcpy(string, "");

  /* Get rid of comments */
  if ((delim = strchr(start, '#')) != NULL) {
    *delim = '\0';
  }

  /* Get rid of starting/trailing spaces and tabs */
  remove_starting_blanks(&start);
  remove_trailing_blanks(start);

  /* Find next blank */
  if ((delim = find_blank(start)) == NULL) {
    delim = &start[strlen(start)];
  }

  /* Extract keyword */
  strncpy(keyword, start, delim - start);
  keyword[delim - start] = '\0';
  if (keyword[0] != '\0') {
    param_count++;
  } else {
    free(linecopy);
    return param_count;
  }

  /* Go to next parameter */
  start = delim;
  remove_starting_blanks(&start);

  /* Find next blank or double quote */
  if (*start == '"') {
    start++;
    if ((delim = strchr(start, '"')) == NULL) {
      /* No matching double quote */
      free(linecopy);
      return -1;
    } else {
      /* Make that a space so it gets ignored */
      *delim = ' ';
    }
  } else if ((delim = find_blank(start)) == NULL) {
    delim = &start[strlen(start)];
  }

  /* Extract type or string */
  strncpy(string, start, delim - start);
  string[delim - start] = '\0';
  if (string[0] != '\0') {
    param_count++;
  } else {
    free(linecopy);
    return param_count;
  }

  /* Go to next parameter */
  start = delim;
  remove_starting_blanks(&start);

  /* Do we have more? */
  if (start[0] == '\0') {
    free(linecopy);
    return param_count;
  } else {
    strcpy(type, string);
  }

  /* Find next blank or double quote */
  if (*start == '"') {
    start++;
    if ((delim = strchr(start, '"')) == NULL) {
      /* No matching double quote */
      free(linecopy);
      return -1;
    } else {
      /* Make that a space so it gets ignored */
      *delim = ' ';
    }
  } else if ((delim = find_blank(start)) == NULL) {
    delim = &start[strlen(start)];
  }

  /* Extract string */
  strncpy(string, start, delim - start);
  string[delim - start] = '\0';
  if (string[0] != '\0') {
    param_count++;
  } else {
    free(linecopy);
    return param_count;
  }

  free(linecopy);
  return param_count;
}
