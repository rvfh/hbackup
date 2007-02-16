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

using namespace std;

#include <iostream>
#include <string>
#include <sys/stat.h>

#include "files.h"

int terminating(void) {
  return 0;
}

int main(void) {
  string line;

  cout << "Tools Test" << endl;

  cout << endl << "Test: zcopy" << endl;
  system("dd if=/dev/zero of=zcopy_test bs=1M count=10 status=noxfer 2> /dev/null");
  off_t size_in;
  off_t size_out;
  char  check_in[36];
  char  check_out[36];
  File::zcopy("zcopy_test", "zcopied", &size_in, &size_out, check_in, check_out, 5);
  cout << "In: " << size_in << " bytes, checksum: " << check_in << endl;
  cout << "Out: " << size_out << " bytes, checksum: " << check_out << endl;

  cout << endl << "Test: testDir" << endl;
  cout << "Check test_db dir: "
    << File::testDir("test_db", true) << endl;
  cout << "Check but do not create test_db/0 1 dir: "
    << File::testDir("test_db/0 1", false) << endl;
  cout << "Check and create test_db/0 1 dir: "
    << File::testDir("test_db/0 1", true) << endl;
  cout << "Check test_db/0 1 dir: "
    << File::testDir("test_db/0 1", false) << endl;

  cout << endl << "Test: testReg" << endl;
  cout << "Check but do not create test_db/0 1/a b file: "
    << File::testReg("test_db/0 1/a b", false) << endl;
  cout << "Check and create test_db/0 1/a b file: "
    << File::testReg("test_db/0 1/a b", true) << endl;
  cout << "Check test_db/0 1/a b file: "
    << File::testReg("test_db/0 1/a b", 0) << endl;

  cout << endl << "Test: typeLetter" << endl;
  printf("File   : %c\n", File::typeLetter(S_IFREG));
  printf("Dir    : %c\n", File::typeLetter(S_IFDIR));
  printf("Char   : %c\n", File::typeLetter(S_IFCHR));
  printf("Block  : %c\n", File::typeLetter(S_IFBLK));
  printf("FIFO   : %c\n", File::typeLetter(S_IFIFO));
  printf("Link   : %c\n", File::typeLetter(S_IFLNK));
  printf("Socket : %c\n", File::typeLetter(S_IFSOCK));
  printf("Unknown: %c\n", File::typeLetter(0));

  cout << endl << "Test: typeMode" << endl;
  printf("File   : 0%06o\n", File::typeMode('f'));
  printf("Dir    : 0%06o\n", File::typeMode('d'));
  printf("Char   : 0%06o\n", File::typeMode('c'));
  printf("Block  : 0%06o\n", File::typeMode('b'));
  printf("FIFO   : 0%06o\n", File::typeMode('p'));
  printf("Link   : 0%06o\n", File::typeMode('l'));
  printf("Socket : 0%06o\n", File::typeMode('s'));
  printf("Unknown: 0%06o\n", File::typeMode('?'));

  cout << endl << "Test: params_readline" << endl;
  char   keyword[256];
  char   type[256];
  string s;

  line = "";
  cout << "Read " << params_readline(line, keyword, type, &s)
    << " parameters from " << line << ": '" << keyword << "' '" << type
    << "' '" << s << "'" <<  endl;
  line = "# Normal comment";
  cout << "Read " << params_readline(line, keyword, type, &s)
    << " parameters from " << line << ": '" << keyword << "' '" << type
    << "' '" << s << "'" <<  endl;
  line = " \t# Displaced comment";
  cout << "Read " << params_readline(line, keyword, type, &s)
    << " parameters from " << line << ": '" << keyword << "' '" << type
    << "' '" << s << "'" <<  endl;
  line = "\tkey # Comment";
  cout << "Read " << params_readline(line, keyword, type, &s)
    << " parameters from " << line << ": '" << keyword << "' '" << type
    << "' '" << s << "'" <<  endl;
  line = "key\t \"string\" # Comment";
  cout << "Read " << params_readline(line, keyword, type, &s)
    << " parameters from " << line << ": '" << keyword << "' '" << type
    << "' '" << s << "'" <<  endl;
  line = "key \ttype\t\"string\" # Comment";
  cout << "Read " << params_readline(line, keyword, type, &s)
    << " parameters from " << line << ": '" << keyword << "' '" << type
    << "' '" << s << "'" <<  endl;
  line = "key\t string \t# Comment";
  cout << "Read " << params_readline(line, keyword, type, &s)
    << " parameters from " << line << ": '" << keyword << "' '" << type
    << "' '" << s << "'" <<  endl;
  line = "key \ttype\t string # Comment";
  cout << "Read " << params_readline(line, keyword, type, &s)
    << " parameters from " << line << ": '" << keyword << "' '" << type
    << "' '" << s << "'" <<  endl;
  line = "key \ttype \t\"string # Comment";
  cout << "Read " << params_readline(line, keyword, type, &s)
    << " parameters from " << line << ": '" << keyword << "' '" << type
    << "' '" << s << "'" <<  endl;
  line = "key \ttype string\" # Comment";
  cout << "Read " << params_readline(line, keyword, type, &s)
    << " parameters from " << line << ": '" << keyword << "' '" << type
    << "' '" << s << "'" <<  endl;
  line = "key\t \"this is\ta \tstring\" # Comment";
  cout << "Read " << params_readline(line, keyword, type, &s)
    << " parameters from " << line << ": '" << keyword << "' '" << type
    << "' '" << s << "'" <<  endl;
  line = "key \ttype\t \"this is\ta \tstring\" # Comment";
  cout << "Read " << params_readline(line, keyword, type, &s)
    << " parameters from " << line << ": '" << keyword << "' '" << type
    << "' '" << s << "'" <<  endl;

  metadata_t metadata;
  struct tm *time;

  cout << "\nmetadata_get" << endl;
  metadata_get("test/testfile", &metadata);
  time = localtime(&metadata.mtime);

  printf(" * type: 0x%08x\n", metadata.type);
  printf(" * size: %u\n", (unsigned int) metadata.size);

  printf(" * mtime: %04u-%02u-%02u %2u:%02u:%02u\n",
    time->tm_year + 1900, time->tm_mon + 1, time->tm_mday,
    time->tm_hour, time->tm_min, time->tm_sec);

  printf(" * uid: %u\n", metadata.uid);
  printf(" * gid: %u\n", metadata.gid);
  printf(" * mode: 0x%08x\n", metadata.mode);

  return 0;
}
