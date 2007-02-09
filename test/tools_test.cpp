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

/* TODO Not all functions are tested */

#include <iostream>
#include "tools.cpp"

using namespace std;

int terminating(void) {
  return 0;
}

int main(void) {
  string line;

  line = "/this/is/a/line";
  cout << "Check slashes to '" << line << "': ";
  no_trailing_slash(&line);
  cout << "'" << line << "'" << endl;

  line = "/this/is/a/line/";
  cout << "Check slashes to '" << line << "': ";
  no_trailing_slash(&line);
  cout << "'" << line << "'" << endl;

  line = "/this/is/a/line/////////////";
  cout << "Check slashes to '" << line << "': ";
  no_trailing_slash(&line);
  cout << "'" << line << "'" << endl;

  line = "This is a text which I like";
  cout << "Converting '" << line << "' to lower case" << endl;
  strtolower(&line);
  cout << "-> gives '" << line << "'" << endl;

  line = "C:\\Program Files\\HBackup\\hbackup.EXE";
  cout << "Converting '" << line << "' to linux style" << endl;
  pathtolinux(&line);
  cout << "-> gives '" << line << "'" << endl;
  cout << "Then to lower case" << endl;
  strtolower(&line);
  cout << "-> gives '" << line << "'" << endl;

  printf("Test: RingBuffer\n");
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

  printf("Test: zcopy\n");
  system("dd if=/dev/zero of=zcopy_test bs=1M count=100 status=noxfer 2> /dev/null");
  off_t size_in;
  off_t size_out;
  char  check_in[36];
  char  check_out[36];
  zcopy("zcopy_test", "zcopied", &size_in, &size_out, check_in, check_out, 5);
  cout << "In: " << size_in << " bytes, checksum: " << check_in << endl;
  cout << "Out: " << size_out << " bytes, checksum: " << check_out << endl;

  return 0;
}