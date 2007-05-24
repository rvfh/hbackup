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

using namespace std;

#include <iostream>
#include <vector>
#include <string>
#include <sys/stat.h>

#include "files.h"

using namespace hbackup;

int terminating(void) {
  return 0;
}

int main(void) {
  string  line;
  File*   readfile;
  File*   writefile;

  cout << "Tools Test" << endl;

  cout << endl << "Test: zcopy" << endl;
  system("dd if=/dev/zero of=test/zcopy_source bs=1M count=10 status=noxfer 2> /dev/null");
  long long size_in  = 125;
  long long size_out = 250;
  string  check_in  = "bart";
  string  check_out = "ernest";
  File::zcopy("test/zcopy_source", "test/zcopy_dest", &size_in, &size_out,
    &check_in, &check_out, 5);
  cout << "In: " << size_in << " bytes, checksum: " << check_in << endl;
  cout << "Out: " << size_out << " bytes, checksum: " << check_out << endl;
  size_in  = 125;
  size_out = 250;
  check_in  = "bart";
  check_out = "ernest";
  File::zcopy("test/zcopy_source", "test/zcopy_dest", &size_in, &size_out,
    &check_in, &check_out, 0);
  cout << "In: " << size_in << " bytes, checksum: " << check_in << endl;
  cout << "Out: " << size_out << " bytes, checksum: " << check_out << endl;

  cout << endl << "Test: file read" << endl;
  readfile = new File(".", "test/zcopy_source");
  if (readfile->open("r")) {
    cout << "Error opening file" << endl;
  } else {
    unsigned char buffer[File::chunk];
    size_t read_size = 0;
    do {
      size_t size = readfile->read(buffer, File::chunk);
      if (size < 0) {
        cout << "broken by read" << endl;
        break;
      }
      read_size += size;
    } while (! readfile->eof());
    if (readfile->close()) cout << "Error closing file" << endl;
    cout << "read size: " << read_size
      << " (" << readfile->size() << " -> " <<  readfile->dsize()
      << "), checksum: " << readfile->checksum() << endl;
  }
  delete readfile;

  cout << endl << "Test: file copy (read + write)" << endl;
  system("dd if=/dev/zero of=test/zcopy_source bs=1M count=10 status=noxfer 2> /dev/null");
  readfile = new File(".", "test/zcopy_source");
  writefile = new File(".", "test/zcopy_dest");
  if (readfile->open("r") || writefile->open("w")) {
    cout << "Error opening file" << endl;
  } else {
    unsigned char buffer[File::chunk];
    size_t read_size = 0;
    size_t write_size = 0;
    do {
      size_t size = readfile->read(buffer, File::chunk);
      if (size < 0) {
        cout << "broken by read" << endl;
        break;
      }
      read_size += size;
      size = writefile->write(buffer, size, readfile->eof());
      if (size < 0) {
        cout << "broken by write" << endl;
        break;
      }
      write_size += size;
    } while (! readfile->eof());
    if (readfile->close()) cout << "Error closing read file" << endl;
    if (writefile->close()) cout << "Error closing write file" << endl;
    cout << "read size: " << read_size
      << " (" << readfile->size() << " -> " <<  readfile->dsize()
      << "), checksum: " << readfile->checksum() << endl;
    cout << "write size: " << write_size
      << " (" << writefile->dsize() << " -> " <<  writefile->size()
      << "), checksum: " << writefile->checksum() << endl;
  }
  delete readfile;
  delete writefile;

  cout << endl << "Test: file compress (read + compress write)" << endl;
  readfile = new File(".", "test/zcopy_source");
  writefile = new File(".", "test/zcopy_dest");
  if (readfile->open("r") || writefile->open("w", 5)) {
    cout << "Error opening file" << endl;
  } else {
    unsigned char buffer[File::chunk];
    size_t read_size = 0;
    size_t write_size = 0;
    do {
      size_t size = readfile->read(buffer, File::chunk);
      if (size < 0) {
        cout << "broken by read" << endl;
        break;
      }
      read_size += size;
      size = writefile->write(buffer, size, readfile->eof());
      if (size < 0) {
        cout << "broken by write" << endl;
        break;
      }
      write_size += size;
    } while (! readfile->eof());
    if (readfile->close()) cout << "Error closing read file" << endl;
    if (writefile->close()) cout << "Error closing write file" << endl;
    cout << "read size: " << read_size
      << " (" << readfile->size() << " -> " <<  readfile->dsize()
      << "), checksum: " << readfile->checksum() << endl;
    cout << "write size: " << write_size
      << " (" << writefile->dsize() << " -> " <<  writefile->size()
      << "), checksum: " << writefile->checksum() << endl;
  }
  delete readfile;
  delete writefile;

  cout << endl << "Test: file uncompress (uncompress read + write)" << endl;
  readfile = new File(".", "test/zcopy_dest");
  writefile = new File(".", "test/zcopy_source");
  if (readfile->open("r", 1) || writefile->open("w")) {
    cout << "Error opening file" << endl;
  } else {
    unsigned char buffer[File::chunk];
    size_t read_size = 0;
    size_t write_size = 0;
    do {
      size_t size = readfile->read(buffer, File::chunk);
      if (size < 0) {
        cout << "broken by read" << endl;
        break;
      }
      read_size += size;
      size = writefile->write(buffer, size, readfile->eof());
      if (size < 0) {
        cout << "broken by write" << endl;
        break;
      }
      write_size += size;
    } while (! readfile->eof());
    if (readfile->close()) cout << "Error closing read file" << endl;
    if (writefile->close()) cout << "Error closing write file" << endl;
    cout << "read size: " << read_size
      << " (" << readfile->size() << " -> " <<  readfile->dsize()
      << "), checksum: " << readfile->checksum() << endl;
    cout << "write size: " << write_size
      << " (" << writefile->dsize() << " -> " <<  writefile->size()
      << "), checksum: " << writefile->checksum() << endl;
  }
  delete readfile;
  delete writefile;

  cout << endl << "Test: file compress (read + compress write)"
    << endl;
  readfile = new File(".", "test/zcopy_source");
  writefile = new File(".", "test/zcopy_dest");
  if (readfile->open("r") || writefile->open("w", 5)) {
    cout << "Error opening file" << endl;
  } else {
    unsigned char buffer[File::chunk];
    size_t read_size = 0;
    size_t write_size = 0;
    do {
      size_t size = readfile->read(buffer, File::chunk);
      if (size < 0) {
        cout << "broken by read" << endl;
        break;
      }
      read_size += size;
      size = writefile->write(buffer, size, readfile->eof());
      if (size < 0) {
        cout << "broken by write" << endl;
        break;
      }
      write_size += size;
    } while (! readfile->eof());
    if (readfile->close()) cout << "Error closing read file" << endl;
    if (writefile->close()) cout << "Error closing write file" << endl;
    cout << "read size: " << read_size
      << " (" << readfile->size() << " -> " <<  readfile->dsize()
      << "), checksum: " << readfile->checksum() << endl;
    cout << "write size: " << write_size
      << " (" << writefile->dsize() << " -> " <<  writefile->size()
      << "), checksum: " << writefile->checksum() << endl;
  }
  cout << endl
    << "Test: file recompress (uncompress read + compress write), no closing"
    << endl;
  {
    File* swap = readfile;
    readfile = writefile;
    writefile = swap;
  }
  if (readfile->open("r", 1) || writefile->open("w", 5)) {
    cout << "Error opening file" << endl;
  } else {
    unsigned char buffer[File::chunk];
    size_t read_size = 0;
    size_t write_size = 0;
    do {
      size_t size = readfile->read(buffer, File::chunk);
      if (size < 0) {
        cout << "broken by read" << endl;
        break;
      }
      read_size += size;
      size = writefile->write(buffer, size, readfile->eof());
      if (size < 0) {
        cout << "broken by write" << endl;
        break;
      }
      write_size += size;
    } while (! readfile->eof());
    if (readfile->close()) cout << "Error closing read file" << endl;
    if (writefile->close()) cout << "Error closing write file" << endl;
    cout << "read size: " << read_size
      << " (" << readfile->size() << " -> " <<  readfile->dsize()
      << "), checksum: " << readfile->checksum() << endl;
    cout << "write size: " << write_size
      << " (" << writefile->dsize() << " -> " <<  writefile->size()
      << "), checksum: " << writefile->checksum() << endl;
  }
  delete readfile;
  delete writefile;


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
  printf("Zero   : %c\n", File::typeLetter(0));
  printf("Unknown: %c\n", File::typeLetter(-1));

  cout << endl << "Test: typeMode" << endl;
  printf("File   : 0%06o\n", File::typeMode('f'));
  printf("Dir    : 0%06o\n", File::typeMode('d'));
  printf("Char   : 0%06o\n", File::typeMode('c'));
  printf("Block  : 0%06o\n", File::typeMode('b'));
  printf("FIFO   : 0%06o\n", File::typeMode('p'));
  printf("Link   : 0%06o\n", File::typeMode('l'));
  printf("Socket : 0%06o\n", File::typeMode('s'));
  printf("Unknown: 0%06o\n", File::typeMode('?'));

  cout << "\nmetadata" << endl;
  {
    struct tm *time;
    File file_data("test/testfile");
    time_t file_time = file_data.mtime();
    time = localtime(&file_time);
    cout << "Line: " << file_data.line(true) << endl;;
    printf(" * type: 0x%08x\n", file_data.type());
    printf(" * mtime (local): %04u-%02u-%02u %2u:%02u:%02u\n",
      time->tm_year + 1900, time->tm_mon + 1, time->tm_mday,
      time->tm_hour, time->tm_min, time->tm_sec);
  }

  cout << "\nline constructor" << endl;
  {
    File file_data("test/testlink");
    file_data.setPrefix("prefix");
    cout << "Line: " << file_data.line(true) << endl;;
    char* new_line = new char[80];
    strcpy(new_line, (file_data.line(true) + "\t").c_str());
    File file_line(new_line, 80);
    cout << "Line: " << file_line.line() << endl;;
  }

  cout << "\nreadline" << endl;
  vector<string> *params;

  // Start simple: one argument
  line = "a";
  params = new vector<string>;
  cout << "readline(" << line << "): " << File::decodeLine(line, *params) << endl;
  for (unsigned int i = 0; i < params->size(); i++) {
    cout << (*params)[i] << endl;
  }
  cout << endl;
  delete params;

  // Two arguments, test blanks
  line = " \ta \tb";
  params = new vector<string>;
  cout << "readline(" << line << "): " << File::decodeLine(line, *params) << endl;
  for (unsigned int i = 0; i < params->size(); i++) {
    cout << (*params)[i] << endl;
  }
  cout << endl;
  delete params;

  // Not single character argument
  line = "\t ab";
  params = new vector<string>;
  cout << "readline(" << line << "): " << File::decodeLine(line, *params)
    << endl;
  for (unsigned int i = 0; i < params->size(); i++) {
    cout << (*params)[i] << endl;
  }
  cout << endl;
  delete params;

  // Two of them
  line = "\t ab cd";
  params = new vector<string>;
  cout << "readline(" << line << "): " << File::decodeLine(line, *params)
    << endl;
  for (unsigned int i = 0; i < params->size(); i++) {
    cout << (*params)[i] << endl;
  }
  cout << endl;
  delete params;

  // Three, with comment
  line = "\t ab cd\tef # blah";
  params = new vector<string>;
  cout << "readline(" << line << "): " << File::decodeLine(line, *params)
    << endl;
  for (unsigned int i = 0; i < params->size(); i++) {
    cout << (*params)[i] << endl;
  }
  cout << endl;
  delete params;

  // Single quotes
  line = "\t 'ab' 'cd'\t'ef' # blah";
  params = new vector<string>;
  cout << "readline(" << line << "): " << File::decodeLine(line, *params)
    << endl;
  for (unsigned int i = 0; i < params->size(); i++) {
    cout << (*params)[i] << endl;
  }
  cout << endl;
  delete params;

  // And double quotes
  line = "\t \"ab\" 'cd'\t\"ef\" # blah";
  params = new vector<string>;
  cout << "readline(" << line << "): " << File::decodeLine(line, *params)
    << endl;
  for (unsigned int i = 0; i < params->size(); i++) {
    cout << (*params)[i] << endl;
  }
  cout << endl;
  delete params;

  // With blanks in quotes
  line = "\t ab cd\tef 'gh ij\tkl' \"mn op\tqr\" \t# blah";
  params = new vector<string>;
  cout << "readline(" << line << "): " << File::decodeLine(line, *params)
    << endl;
  for (unsigned int i = 0; i < params->size(); i++) {
    cout << (*params)[i] << endl;
  }
  cout << endl;
  delete params;

  // With quotes in quotes
  line = "\t ab cd\tef 'gh \"ij\\\'\tkl' \"mn 'op\\\"\tqr\" \t# blah";
  params = new vector<string>;
  cout << "readline(" << line << "): " << File::decodeLine(line, *params)
    << endl;
  for (unsigned int i = 0; i < params->size(); i++) {
    cout << (*params)[i] << endl;
  }
  cout << endl;
  delete params;

  // With escape characters
  line = "\t a\\b cd\tef 'g\\h \"ij\\\'\tkl' \"m\\n 'op\\\"\tqr\" \t# blah";
  params = new vector<string>;
  cout << "readline(" << line << "): " << File::decodeLine(line, *params)
    << endl;
  for (unsigned int i = 0; i < params->size(); i++) {
    cout << (*params)[i] << endl;
  }
  cout << endl;
  delete params;

  // Missing ending single quote
  line = "\t a\\b cd\tef 'g\\h \"ij\\\'\tkl";
  params = new vector<string>;
  cout << "readline(" << line << "): " << File::decodeLine(line, *params)
    << endl;
  for (unsigned int i = 0; i < params->size(); i++) {
    cout << (*params)[i] << endl;
  }
  cout << endl;
  delete params;

  // Missing ending double quote
  line = "\t a\\b cd\tef 'g\\h \"ij\\\'\tkl' \"m\\n 'op\\\"\tqr";
  params = new vector<string>;
  cout << "readline(" << line << "): " << File::decodeLine(line, *params)
    << endl;
  for (unsigned int i = 0; i < params->size(); i++) {
    cout << (*params)[i] << endl;
  }
  cout << endl;
  delete params;

  return 0;
}
