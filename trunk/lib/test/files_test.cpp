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

int verbosity(void) {
  return 0;
}

int terminating(void) {
  return 0;
}

void showFile(const Node &g) {
  switch (g.type()) {
  case 'f': {
    File2 *f = new File2(g);
    cout << "Name: " << f->name()
      << ", type = " << f->type()
      << ", mtime = " << (f->mtime() != 0)
      << ", size = " << f->size()
      << ", uid = " << (int)(f->uid() != 0)
      << ", gid = " << (int)(f->gid() != 0)
      << ", mode = " << f->mode()
      << endl;
    delete f; }
    break;
  case 'l': {
    Link *l = new Link(g);
    cout << "Name: " << l->name()
      << ", type = " << l->type()
      << ", mtime = " << (l->mtime() != 0)
      << ", size = " << l->size()
      << ", uid = " << (int)(l->uid() != 0)
      << ", gid = " << (int)(l->gid() != 0)
      << ", mode = " << l->mode()
      << ", link = " << l->link()
      << endl;
    delete l; }
    break;
  case 'd': {
    Directory *d = new Directory(g);
    cout << "Name: " << d->name()
      << ", type = " << d->type()
      << ", mtime = " << (d->mtime() != 0)
      << ", size = " << d->size()
      << ", uid = " << (int)(d->uid() != 0)
      << ", gid = " << (int)(d->gid() != 0)
      << ", mode = " << d->mode()
      << endl;
    d->createList();
    NodeListElement* entry = d->entries_head();
    cout << "Listing " << d->entries() << " entries:" << endl;
    while (entry != NULL) {
      cout << " -> name: " << entry->payload()->name() << endl;
      entry = entry->next();
    }
    delete d; }
    break;
  default:
    cout << "Unknown file type: " << g.type() << endl;
  }
}

int main(void) {
  string  line;

  cout << "Tools Test" << endl;

  cout << endl << "Test: zcopy" << endl;
  system("dd if=/dev/zero of=test1/zcopy_source bs=1M count=10 status=noxfer 2> /dev/null");
  long long size_in  = 125;
  long long size_out = 250;
  string  check_in  = "bart";
  string  check_out = "ernest";
  File::zcopy("test1/zcopy_source", "test1/zcopy_dest", &size_in, &size_out,
    &check_in, &check_out, 5);
  cout << "In: " << size_in << " bytes, checksum: " << check_in << endl;
  cout << "Out: " << size_out << " bytes, checksum: " << check_out << endl;
  size_in  = 125;
  size_out = 250;
  check_in  = "bart";
  check_out = "ernest";
  File::zcopy("test1/zcopy_source", "test1/zcopy_dest", &size_in, &size_out,
    &check_in, &check_out, 0);
  cout << "In: " << size_in << " bytes, checksum: " << check_in << endl;
  cout << "Out: " << size_out << " bytes, checksum: " << check_out << endl;

  Stream* readfile;
  Stream* writefile;

  cout << endl << "Test: file read" << endl;
  readfile = new Stream("test1/zcopy_source");
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
  system("dd if=/dev/zero of=test1/zcopy_source bs=1M count=10 status=noxfer 2> /dev/null");
  readfile = new Stream("test1/zcopy_source");
  writefile = new Stream("test1/zcopy_dest");
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
  readfile = new Stream("test1/zcopy_source");
  writefile = new Stream("test1/zcopy_dest");
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
  readfile = new Stream("test1/zcopy_dest");
  writefile = new Stream("test1/zcopy_source");
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
  readfile = new Stream("test1/zcopy_source");
  writefile = new Stream("test1/zcopy_dest");
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
    Stream* swap = readfile;
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

  cout << endl << "Test: computeChecksum" << endl;
  readfile = new Stream("test1/testfile");
  if (readfile->computeChecksum()) {
    cout << "Error computing checksum" << endl;
  } else {
    cout << "Checksum: " << readfile->checksum() << endl;
  }
  delete readfile;


  cout << "\nmetadata" << endl;
  {
    struct tm *time;
    File file_data("test1/testfile");
    time_t file_time = file_data.mtime();
    time = localtime(&file_time);
    cout << "Line: " << file_data.line(true) << endl;;
    printf(" * mtime (local): %04u-%02u-%02u %2u:%02u:%02u\n",
      time->tm_year + 1900, time->tm_mon + 1, time->tm_mday,
      time->tm_hour, time->tm_min, time->tm_sec);
  }

  cout << "\nline constructor" << endl;
  {
    File file_data("test1/testlink");
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

  // New age preparation test
  Node *g;
  g = new Node("test1/testfile");
  showFile(*g);
  delete g;
  g = new Node("test1/testlink");
  showFile(*g);
  delete g;
  g = new Node("test1/testdir");
  showFile(*g);
  delete g;
  g = new Node("test1/subdir");
  showFile(*g);
  delete g;

  cout << endl << "Validity tests" << endl;
  cout << "File is file? " << File2("test1/testfile").isValid() << endl;
  cout << "File is dir? " << Directory("test1/testfile").isValid() << endl;
  cout << "File is link? " << Link("test1/testfile").isValid() << endl;
  cout << "Dir is file? " << File2("test1/testdir").isValid() << endl;
  cout << "Dir is dir? " << Directory("test1/testdir").isValid() << endl;
  cout << "Dir is link? " << Link("test1/testdir").isValid() << endl;
  cout << "Link is file? " << File2("test1/testlink").isValid() << endl;
  cout << "Link is dir? " << Directory("test1/testlink").isValid() << endl;
  cout << "Link is link? " << Link("test1/testlink").isValid() << endl;

  cout << endl << "Creation tests" << endl;
  cout << "File is file? " << File2("test1/touchedfile").isValid() << endl;
  cout << "File is dir? " << Directory("test1/touchedfile").isValid() << endl;
  cout << "File is link? " << Link("test1/touchedfile").isValid() << endl;
  cout << "Dir is file? " << File2("test1/toucheddir").isValid() << endl;
  cout << "Dir is dir? " << Directory("test1/toucheddir").isValid() << endl;
  cout << "Dir is link? " << Link("test1/toucheddir").isValid() << endl;
  cout << "Link is file? " << File2("test1/touchedlink").isValid() << endl;
  cout << "Link is dir? " << Directory("test1/touchedlink").isValid() << endl;
  cout << "Link is link? " << Link("test1/touchedlink").isValid() << endl;

  cout << "Create" << endl;
  if (File2("test1/touchedfile").create())
    cout << "failed to create file" << endl;
  if (Directory("test1/toucheddir").create())
    cout << "failed to create dir" << endl;

  cout << "File is file? " << File2("test1/touchedfile").isValid() << endl;
  cout << "File is dir? " << Directory("test1/touchedfile").isValid() << endl;
  cout << "File is link? " << Link("test1/touchedfile").isValid() << endl;
  cout << "Dir is file? " << File2("test1/toucheddir").isValid() << endl;
  cout << "Dir is dir? " << Directory("test1/toucheddir").isValid() << endl;
  cout << "Dir is link? " << Link("test1/toucheddir").isValid() << endl;
  cout << "Link is file? " << File2("test1/touchedlink").isValid() << endl;
  cout << "Link is dir? " << Directory("test1/touchedlink").isValid() << endl;
  cout << "Link is link? " << Link("test1/touchedlink").isValid() << endl;

  cout << "Create again" << endl;
  if (File2("test1/touchedfile").create())
    cout << "failed to create file" << endl;
  if (Directory("test1/toucheddir").create())
    cout << "failed to create dir" << endl;

  cout << "File is file? " << File2("test1/touchedfile").isValid() << endl;
  cout << "File is dir? " << Directory("test1/touchedfile").isValid() << endl;
  cout << "File is link? " << Link("test1/touchedfile").isValid() << endl;
  cout << "Dir is file? " << File2("test1/toucheddir").isValid() << endl;
  cout << "Dir is dir? " << Directory("test1/toucheddir").isValid() << endl;
  cout << "Dir is link? " << Link("test1/toucheddir").isValid() << endl;
  cout << "Link is file? " << File2("test1/touchedlink").isValid() << endl;
  cout << "Link is dir? " << Directory("test1/touchedlink").isValid() << endl;
  cout << "Link is link? " << Link("test1/touchedlink").isValid() << endl;

  return 0;
}
