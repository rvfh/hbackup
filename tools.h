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

#ifndef TOOLS_H
#define TOOLS_H

/* Make sure string finishes without slash */
extern void no_trailing_slash(char *string);

/* Convert string to lower case */
extern void strtolower(char *string);

/* Convert path to UNIX style */
extern void pathtolinux(char *path);

/* Test whether dir exists, create it if requested */
extern int testdir(const char *path, int create);

/* Test whether file exists, create it if requested */
extern int testfile(const char *path, int create);

#endif
