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

#include <iostream>
#include "tools.cpp"

using namespace std;

int terminating(void) {
  return 0;
}

int main(void) {
  string line;

  cout << "Tools Test" << endl;

  cout << endl << "Test: no_trailing_slash" << endl;
  line = "/this/is/a/line";
  cout << "Check slashes to '" << line << "': ";
  no_trailing_slash(line);
  cout << "'" << line << "'" << endl;

  line = "/this/is/a/line/";
  cout << "Check slashes to '" << line << "': ";
  no_trailing_slash(line);
  cout << "'" << line << "'" << endl;

  line = "/this/is/a/line/////////////";
  cout << "Check slashes to '" << line << "': ";
  no_trailing_slash(line);
  cout << "'" << line << "'" << endl;

  cout << endl << "Test: strtolower" << endl;
  line = "This is a text which I like";
  cout << "Converting '" << line << "' to lower case" << endl;
  strtolower(line);
  cout << "-> gives '" << line << "'" << endl;

  cout << endl << "Test: pathtolinux" << endl;
  line = "C:\\Program Files\\HBackup\\hbackup.EXE";
  cout << "Converting '" << line << "' to linux style" << endl;
  pathtolinux(line);
  cout << "-> gives '" << line << "'" << endl;
  cout << "Then to lower case" << endl;
  strtolower(line);
  cout << "-> gives '" << line << "'" << endl;

  cout << endl << "Test: RingBuffer" << endl;
  char  read_buffer[15];
  int   read_size;
  RingBuffer<char> ring_buffer(10);

  cout << "Buffer size used: " << ring_buffer.size() << endl;
  cout << "Wrote: " << ring_buffer.write("1234567890", 5) << endl;
  cout << "Buffer size used: " << ring_buffer.size() << endl;
  cout << "Wrote: " << ring_buffer.write("ABCDEFGHIJKLM", 7) << endl;
  cout << "Buffer size used: " << ring_buffer.size() << endl;
  read_size = ring_buffer.read(read_buffer, 7);
  read_buffer[read_size] = '\0';
  cout << "Read: " << read_size << ", " << read_buffer << endl;
  cout << "Buffer size used: " << ring_buffer.size() << endl;
  read_size = ring_buffer.read(read_buffer, 7);
  read_buffer[read_size] = '\0';
  cout << "Read: " << read_size << ", " << read_buffer << endl;
  cout << "Buffer size used: " << ring_buffer.size() << endl;

  cout << endl << "Test: zcopy (and thus getchecksum)" << endl;
  system("dd if=/dev/zero of=zcopy_test bs=1M count=10 status=noxfer 2> /dev/null");
  off_t size_in;
  off_t size_out;
  char  check_in[36];
  char  check_out[36];
  zcopy("zcopy_test", "zcopied", &size_in, &size_out, check_in, check_out, 5);
  cout << "In: " << size_in << " bytes, checksum: " << check_in << endl;
  cout << "Out: " << size_out << " bytes, checksum: " << check_out << endl;

  cout << endl << "Test: testdir" << endl;
  cout << "Check test_db dir: " << testdir("test_db", true) << endl;
  cout << "Check but do not create test_db/0 1 dir: "
    << testdir("test_db/0 1", false) << endl;
  cout << "Check and create test_db/0 1 dir: "
    << testdir("test_db/0 1", true) << endl;
  cout << "Check test_db/0 1 dir: "
    << testdir("test_db/0 1", false) << endl;

  cout << endl << "Test: testfile" << endl;
  cout << "Check but do not create test_db/0 1/a b file: "
    << testfile("test_db/0 1/a b", false) << endl;
  cout << "Check and create test_db/0 1/a b file: "
    << testfile("test_db/0 1/a b", true) << endl;
  cout << "Check test_db/0 1/a b file: "
    << testfile("test_db/0 1/a b", 0) << endl;

  cout << endl << "Test: type_letter" << endl;
  printf("File   : %c\n", type_letter(S_IFREG));
  printf("Dir    : %c\n", type_letter(S_IFDIR));
  printf("Char   : %c\n", type_letter(S_IFCHR));
  printf("Block  : %c\n", type_letter(S_IFBLK));
  printf("FIFO   : %c\n", type_letter(S_IFIFO));
  printf("Link   : %c\n", type_letter(S_IFLNK));
  printf("Socket : %c\n", type_letter(S_IFSOCK));
  printf("Unknown: %c\n", type_letter(0));

  cout << endl << "Test: type_mode" << endl;
  printf("File   : 0%06o\n", type_mode('f'));
  printf("Dir    : 0%06o\n", type_mode('d'));
  printf("Char   : 0%06o\n", type_mode('c'));
  printf("Block  : 0%06o\n", type_mode('b'));
  printf("FIFO   : 0%06o\n", type_mode('p'));
  printf("Link   : 0%06o\n", type_mode('l'));
  printf("Socket : 0%06o\n", type_mode('s'));
  printf("Unknown: 0%06o\n", type_mode('?'));

  cout << endl << "Test: getdir" << endl;
  string  path;
  cout << "Check test_db/data dir: " << testdir("test_db/data", true) << endl;
  testfile("test_db/data/.nofiles", true);
  testdir("test_db/data/fe", true);
  testfile("test_db/data/fe/.nofiles", true);
  testfile("test_db/data/fe/test4", true);
  testdir("test_db/data/fe/dc", true);
  testfile("test_db/data/fe/dc/.nofiles", true);
  testdir("test_db/data/fe/ba", true);
  testdir("test_db/data/fe/ba/test1", true);
  testdir("test_db/data/fe/98", true);
  testdir("test_db/data/fe/98/test2", true);
  cout << "febatest1 status: " << getdir("test_db", "febatest1", path)
    << ", path: " << path << endl;
  cout << "fe98test2 status: " << getdir("test_db", "fe98test2", path)
    << ", path: " << path << endl;
  cout << "fe98test3 status: " << getdir("test_db", "fe98test3", path)
    << ", path: " << path << endl;
  cout << "fetest4 status: " << getdir("test_db", "fetest4", path)
    << ", path: " << path << endl;
  cout << "fedc76test5 status: " << getdir("test_db", "fedc76test5", path)
    << ", path: " << path << endl;
  testdir("test_db/data/fe/dc/76", true);
  cout << "fedc76test6 status: " << getdir("test_db", "fedc76test6", path)
    << ", path: " << path << endl;
  cout << "fedc76test6 status: " << getdir("test_db", "fedc76test6", path)
    << ", path: " << path << endl;

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

  return 0;
}
