/*
     Copyright (C) 2006-2007  Herve Fache

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

#ifndef HBACKUP_H
#define HBACKUP_H

/* Interface to Hbackup library */
class Hbackup {
  /* Verbosity level */
  static int verbose = 0;
  /* Termination required */
  static bool killed = 0;
public:
  static void setVerbosity(int verbose) {
    _verbose = verbose;
  }
  static int verbosity(void) {
    return _verbose;
  }
  static void setTerminating(bool killed) {
    _killed = killed;
  }
  static bool terminating(void) {
    return _killed;
  }
};

#endif