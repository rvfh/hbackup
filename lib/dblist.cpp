/*
     Copyright (C) 2007  Herve Fache

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
#include <string.h>
#include <errno.h>

using namespace std;

#include "dbdata.h"
#include "dblist.h"
#include "hbackup.h"

using namespace hbackup;

int DbList::load_v2(FILE* readfile) {
  /* Read the active part of the file into memory */
  char          *buffer = NULL;
  size_t        bsize   = 0;
  unsigned int  line    = 0;
  ssize_t       size    = 0;
  int           failed  = 0;

  char*         prefix  = NULL;
  char*         path    = NULL;


  list<DbData>::iterator  db_data;
  bool                    end_found = false;

  errno = 0;
  while (((size = getline(&buffer, &bsize, readfile)) >= 0) && ! failed) {
    char* buffer_last = &buffer[strlen(buffer) - 1];
    // Remove ending '\n'
    if (*buffer_last != '\n') {
      failed = 1;
      break;
    }
    *buffer_last = '\0';
    if (buffer[0] == '#') {
      end_found = true;
      break;
    } else
    if (buffer[0] != '\t') {
      free(prefix);
      prefix = NULL;
      asprintf(&prefix, "%s", buffer);
    } else
    if ((prefix != NULL) && (buffer[1] != '\t')) {
      free(path);
      path = NULL;
      asprintf(&path, "%s", &buffer[1]);
      db_data = end();
    } else if (path != NULL) {
      *buffer_last = '\t';
      char* start  = &buffer[2];
      char* value  = (char *) malloc(size + 1);
      int   failed = 0;
      int   fields = 7;
      // Fields
      time_t    db_time;          // DB time
      char      type;             // file type
      time_t    mtime;            // time of last modification
      long long size;             // on-disk size, in bytes
      uid_t     uid;              // user ID of owner
      gid_t     gid;              // group ID of owner
      mode_t    mode;             // permissions
      char*     checksum = NULL;  // file checksum
      char*     link = NULL;      // what the link points to

      for (int field = 1; field <= fields; field++) {
        // Get tabulation position
        char* delim = strchr(start, '\t');
        if (delim == NULL) {
          failed = 1;
        } else {
          // Get string portion
          strncpy(value, start, delim - start);
          value[delim - start] = '\0';
          /* Extract data */
          switch (field) {
            case 1:   /* DB timestamp */
              if (sscanf(value, "%ld", &db_time) != 1) {
                failed = -1;
              }
              break;
            case 2:   /* Type */
              if (sscanf(value, "%c", &type) != 1) {
                failed = 2;
              } else if (type == '-') {
                fields = 2;
              } else if ((type == 'f') || (type == 'l')) {
                fields++;
              }
              break;
            case 3:   /* Size */
              if (sscanf(value, "%lld", &size) != 1) {
                failed = 2;
              }
              break;
            case 4:   /* Modification time */
              if (sscanf(value, "%ld", &mtime) != 1) {
                failed = 2;
              }
              break;
            case 5:   /* User */
              if (sscanf(value, "%u", &uid) != 1) {
                failed = 2;
              }
              break;
            case 6:   /* Group */
              if (sscanf(value, "%u", &gid) != 1) {
                failed = 2;
              }
              break;
            case 7:   /* Permissions */
              if (sscanf(value, "%o", &mode) != 1) {
                failed = 2;
              }
              break;
            case 8:  /* Checksum or Link */
                if (type == 'f') {
                  checksum = value;;
                } else if (type == 'l') {
                  link = value;;
                }
              break;
          }
          start = delim + 1;
        }
        if (failed) {
          cerr << "dblist: load: file corrupted, line " << line << endl;
          errno = EUCLEAN;
          break;
        }
      }
      if ((type != '-') && (failed == 0)) {
        Node* node;
        switch (type) {
          case 'f':
            node = new File(path, type, mtime, size, uid, gid, mode, checksum);
            break;
          case 'l':
            node = new Link(path, type, mtime, size, uid, gid, mode, link);
            break;
          default:
            node = new Node(path, type, mtime, size, uid, gid, mode);
        }
        push_back(DbData(prefix, path, node));
      }
      free(value);
      // Only take first file data (active)
      path = NULL;
    }
  }
  free(buffer);
  free(prefix);
  free(path);

  if (! end_found) {
    cerr << "dblist: load: file end not found" << endl;
    errno = EUCLEAN;
    failed = -1;
  }

  return failed;
}

int DbList::open(
    const string& path,
    const string& filename) {
  string source_path = path + "/" + filename;
  bool   failed      = false;

  FILE* readfile = fopen(source_path.c_str(), "r");
  if (readfile == NULL) {
    // errno set by fopen
    failed = true;
  } else {
    /* Read the file into memory */
    char    *buffer = NULL;
    size_t  bsize   = 0;

    // errno set by load_v*
    if (getline(&buffer, &bsize, readfile) >= 0) {
      if (buffer[0] != '#') {
        errno = EUCLEAN;
        failed = true;
      } else {
        // Version 2
        failed = load_v2(readfile);
      }
    }
    free(buffer);
    fclose(readfile);
  }
  if (failed) {
    cerr << "dblist: failed to load list: " << strerror(errno) << endl;
    return -1;
  }

  if (verbosity() > 2) {
    cout << " --> Database " << filename << " open (contents: "
      << size() << " file";
    if (size() != 1) {
      cout << "s";
    }
    cout << ")" << endl;
  }
  return 0;
}

int List::open(
    const char* req_mode) {
  const char header[] = "# version 2\n";
  int rc = 0;

  if (Stream::open(req_mode, 0)) {
    rc = -1;
  } else
  if (isWriteable()) {
    if (write(header, strlen(header)) < 0) {
      Stream::close();
      rc = -1;
    }
  } else {
    char*  line = NULL;
    size_t size = 0;
    if (getLine(&line, &size) < 0) {
      Stream::close();
      rc = -1;
    } else
    if (strcmp(line, header)) {
      errno = EUCLEAN;
      rc = -1;
    }
  }
  return rc;
}

int List::close() {
  const char footer[] = "# end\n";
  int rc = 0;

  if (isWriteable()) {
    if (Stream::write(footer, strlen(footer)) < 0) {
      rc = -1;
    }
  }
  if (Stream::close()) {
    rc = -1;
  }
  return rc;
}

int List::getEntry(
    time_t*   timestamp,
    char**    prefix,
    char**    path,
    Node**    node) {
  char*   line  = NULL;
  size_t  lsize = 0;
  ssize_t length;

  // Initialise
  errno   = 0;
  free(*node);
  *node   = NULL;

  bool done = false;

  while (! done) {
    // Get line
    length = getLine(&line, &lsize);
    if (length == 0) {
      errno = EUCLEAN;
      cerr << "unexpected end of file" << endl;
      break;
    }

    // Check line
    length--;
    if ((length < 2) || (line[length] != '\n')) {
      errno = EUCLEAN;
      break;
    }
    line[length] = '\0';

    // End of file
    if (line[0] == '#') {
      return 0;
    }

    // Prefix
    if (line[0] != '\t') {
      if (prefix != NULL) {
        free(*prefix);
        *prefix = NULL;
        asprintf(prefix, "%s", &line[0]);
      }
    } else

    // Path
    if (line[1] != '\t') {
      if (path != NULL) {
        free(*path);
        *path = NULL;
        asprintf(path, "%s", &line[1]);
      }
    } else

    // Data
    {
      if (node != NULL) {
        line[length] = '\t';
        char*     start  = &line[2];
        int       fields = 7;
        // Fields
        char      type;             // file type
        time_t    mtime;            // time of last modification
        long long size;             // on-disk size, in bytes
        uid_t     uid;              // user ID of owner
        gid_t     gid;              // group ID of owner
        mode_t    mode;             // permissions
        char*     checksum = NULL;  // file checksum
        char*     link     = NULL;  // what the link points to

        for (int field = 1; field <= fields; field++) {
          // Get tabulation position
          char* delim = strchr(start, '\t');
          if (delim == NULL) {
            errno = EUCLEAN;
          } else {
            *delim = '\0';
            /* Extract data */
            switch (field) {
              case 1:   /* DB timestamp */
                if ((timestamp != NULL)
                 && sscanf(start, "%ld", timestamp) != 1) {
                  errno = EUCLEAN;
                }
                break;
              case 2:   /* Type */
                if (sscanf(start, "%c", &type) != 1) {
                  errno = EUCLEAN;;
                } else if (type == '-') {
                  fields = 2;
                } else if ((type == 'f') || (type == 'l')) {
                  fields++;
                }
                break;
              case 3:   /* Size */
                if (sscanf(start, "%lld", &size) != 1) {
                  errno = EUCLEAN;;
                }
                break;
              case 4:   /* Modification time */
                if (sscanf(start, "%ld", &mtime) != 1) {
                  errno = EUCLEAN;;
                }
                break;
              case 5:   /* User */
                if (sscanf(start, "%u", &uid) != 1) {
                  errno = EUCLEAN;;
                }
                break;
              case 6:   /* Group */
                if (sscanf(start, "%u", &gid) != 1) {
                  errno = EUCLEAN;;
                }
                break;
              case 7:   /* Permissions */
                if (sscanf(start, "%o", &mode) != 1) {
                  errno = EUCLEAN;;
                }
                break;
              case 8:  /* Checksum or Link */
                  if (type == 'f') {
                    checksum = start;
                  } else if (type == 'l') {
                    link = start;
                  }
                break;
              default:
                errno = EUCLEAN;;
            }
            start = delim + 1;
          }
          if (errno != 0) {
            cerr << "dblist: file corrupted line " << line << endl;
            errno = EUCLEAN;
            break;
          }
        }
        switch (type) {
          case '-':
            *node = NULL;
            break;
          case 'f':
            *node = new File(*path, type, mtime, size, uid, gid, mode,
              checksum);
            break;
          case 'l':
            *node = new Link(*path, type, mtime, size, uid, gid, mode, link);
            break;
          default:
            *node = new Node(*path, type, mtime, size, uid, gid, mode);
        }
      }
      done = true;
    }
  }

  free(line);
  if (errno != 0) {
    if (prefix != NULL) {
      free(*prefix);
      *prefix = NULL;
    }
    if (path != NULL) {
      free(*path);
      *path = NULL;
    }
    if (node != NULL) {
      free(*node);
      *node = NULL;
    }
    return -1;
  }
  return 1;
}

int List::added(
    const char* prefix,
    const char* path,
    const Node* node,
    time_t      timestamp) {
  Stream::write(prefix, strlen(prefix));
  Stream::write("\n\t", 2);
  Stream::write(path, strlen(path));
  Stream::write("\n", 1);
  char* line = NULL;
  int size = asprintf(&line, "\t\t%ld\t%c\t%lld\t%ld\t%u\t%u\t%o",
    (timestamp >= 0) ? timestamp : time(NULL), node->type(), node->size(),
    node->mtime(), node->uid(), node->gid(), node->mode());
  Stream::write(line, size);
  free(line);
  switch (node->type()) {
    case 'f':
      Stream::write("\t", 1);
      {
        const char* checksum = ((File*) node)->checksum();
        Stream::write(checksum, strlen(checksum));
      }
      break;
    case 'l':
      Stream::write("\t", 1);
      {
        const char* link = ((Link*) node)->link();
        Stream::write(link, strlen(link));
      }
  }
  Stream::write("\n", 1);
  return 0;
}

int List::removed(
    const char*   prefix,
    const char*   path,
    time_t        timestamp) {
  Stream::write(prefix, strlen(prefix));
  Stream::write("\n\t", 2);
  Stream::write(path, strlen(path));
  Stream::write("\n", 1);
  char* line = NULL;
  int size = asprintf(&line, "\t\t%ld\t-\n",
    (timestamp >= 0) ? timestamp : time(NULL));
  Stream::write(line, size);
  free(line);
  return 0;
}

int List::copyUntil(
    List&         list,
    const char*   prefix_l,
    const char*   path_l,
    char**        line,
    size_t*       length,
    int*          status) {
// status values:
// - 0: never ran
// - 1: prefix not found
// - 2: prefix found but not path
// - 3: prefix and/or path found

  int   rc             = 0;
  char* exception_line = NULL;
  int   prefix_cmp     = (*status >= 2) ? 0 : 1;
  int   path_cmp;

  while (true) {
    // Read list or get last data
    if ((*status == 1) || (*status == 2)) {
      rc= strlen(*line);
    } else {
      rc = list.getLine(line, length);
    }
    *status = 0;

    // Failed
    if (rc <= 0) {
      // Unexpected end of file
      cerr << "Unexpected end of list" << endl;
      errno = EUCLEAN;
      rc    = -1;
      break;
    }

    // End of file
    if ((*line)[0] == '#') {
      rc = 0;
      break;
    }

    // Check line
    if ((rc < 2) || ((*line)[rc - 1] != '\n')) {
      // Corrupted line
      cerr << "Corrupted line in list" << endl;
      errno = EUCLEAN;
      rc    = -1;
      break;
    }

    // Full copy
    if (prefix_l == NULL) {
      if (write(*line, rc) < 0) {
        // Could not write
        rc = -1;
        break;
      }
    } else

    // Got a prefix
    if ((*line)[0] != '\t') {
      // Compare prefixes
      prefix_cmp = Node::pathCompare(prefix_l, *line);
      if (prefix_cmp > 0)  {
        // Our prefix is here or after, so let's copy
        if (write(*line, rc) < 0) {
          // Could not write
          rc = -1;
          break;
        }
      } else {
        if (prefix_cmp < 0) {
          // Prefix not found
          *status = 1;
          break;
        } else
        if (path_l == NULL) {
          // Looking for prefix, found
          *status = 3;
          break;
        }
      }
    } else

    // Looking for prefix
    if ((path_l == NULL) || (prefix_cmp > 0)) {
      if (write(*line, rc) < 0) {
        // Could not write
        rc = -1;
        break;
      }
    } else

    // Got a path
    if ((*line)[1] != '\t') {
      // Compare paths
      path_cmp = Node::pathCompare(path_l, *line);
      if (path_cmp > 0) {
        // Our path is here or after, so let's copy
        if (write(*line, rc) < 0) {
          // Could not write
          rc = -1;
          break;
        }
      } else {
        if (path_cmp < 0) {
          // Path not found
          *status = 2;
          break;
        } else {
          // Looking for path, found
          *status = 3;
          break;
        }
      }
    } else

    // Got data
    {
      // Deal with exception
      if (exception_line != NULL) {
        // Copy start of line (timestamp)
        char* pos = strchr(&(*line)[2], '\t');
        if (pos == NULL) {
          errno = EUCLEAN;
          rc    = -1;
          break;
        }
        // Write first part from second line
        if (write(*line, pos - *line + 1) < 0) {
          // Could not write
          rc = -1;
          break;
        }
        // Write end of fisrt line
        if (write(exception_line, strlen(exception_line)) < 0) {
          // Could not write
          rc = -1;
          break;
        }
        free(exception_line);
        exception_line = NULL;
      } else
      // Check for exception
      if (strncmp(*line, "\t\t0\t", 4) == 0) {
        // Get only end of line
        asprintf(&exception_line, "%s", &(*line)[4]);
      } else
      // Our prefix and path are after, so let's copy
      if (write(*line, rc) < 0) {
        // Could not write
        rc = -1;
        break;
      }
    }
  }
  free(exception_line);
  return rc;
}

int List::merge(List& list, List& journal) {
  // Check that all files are open
  if (! isOpen() || ! list.isOpen() || ! journal.isOpen()) {
    errno = EBADF;
    return -1;
  }

  // Check open mode
  if (! isWriteable() || list.isWriteable() || journal.isWriteable()) {
    errno = EINVAL;
    return -1;
  }

  int rc      = 1;
  int rc_list = 1;

  // Line read from journal
  char*   j_line    = NULL;
  size_t  j_length  = 0;
  int     j_line_no = 0;

  // List read from list
  char*   l_line    = NULL;
  size_t  l_length  = 0;

  // Current prefix, path and data (from journal)
  char*   prefix      = NULL;
  char*   path        = NULL;
  char*   data        = NULL;

  // Last copyUntil status
  int     status      = 0;

  // Parse journal
  while (rc > 0) {
    int rc_journal = journal.getLine(&j_line, &j_length);
    j_line_no++;

    // Failed
    if (rc <= 0) {
      // Unexpected end of file
      cerr << "Unexpected end of journal, line " << j_line_no << endl;
      errno = EUCLEAN;
      rc = -1;
      break;
    }

    // Check line
    if ((rc_journal < 2) || (j_line[rc_journal - 1] != '\n')) {
      // Corrupted line
      cerr << "Corruption in journal, line " << j_line_no << endl;
      errno = EUCLEAN;
      rc = -1;
      break;
    }

    // End of file
    if (j_line[0] == '#') {
      if (rc_list > 0) {
        rc_list = copyUntil(list, NULL, NULL, &l_line, &l_length, &status);
        if (rc_list < 0) {
          // Error copying list
          cerr << "End of list copy failed" << endl;
          rc = -1;
          break;
        }
      }
      rc = 0;
    } else

    // Got a prefix
    if (j_line[0] != '\t') {
      if (prefix != NULL) {
        // If same prefix, ignore it
        if (strcmp(prefix, j_line) == 0) {
          continue;
        }
        // Check path order
        if (Node::pathCompare(j_line, prefix) < 0) {
          // Cannot go back
          cerr << "Prefix out of order in journal, line " << j_line_no << endl;
          errno = EUCLEAN;
          rc = -1;
          break;
        }
      }
      // Copy new prefix
      free(prefix);
      prefix = NULL;
      asprintf(&prefix, "%s", j_line);
      // No path for this entry yet
      free(path);
      path = NULL;
      // Search/copy list
      if (rc_list > 0) {
        rc_list = copyUntil(list, prefix, NULL, &l_line, &l_length, &status);
        if (rc_list < 0) {
          // Error copying list
          cerr << "Prefix search failed" << endl;
          rc = -1;
          break;
        }
      }
      // Copy journal prefix
      if (write(j_line, rc_journal) < 0) {
        // Could not write
        cerr << "Prefix write failed" << endl;
        rc = -1;
        break;
      }
    } else

    // Got a path
    if (j_line[1] != '\t') {
      // Must have a prefix by now
      if (prefix == NULL) {
        // Did not get a prefix first thing
        cerr << "Prefix missing in journal, line " << j_line_no << endl;
        errno = EUCLEAN;
        rc = -1;
        break;
      }
      // Check path order
      if (path != NULL) {
        if (Node::pathCompare(j_line, path) < 0) {
          // Cannot go back
          cerr << "Path out of order in journal, line " << j_line_no << endl;
          errno = EUCLEAN;
          rc = -1;
          break;
        }
      }
      // Copy new path
      free(path);
      path = NULL;
      asprintf(&path, "%s", j_line);
      // Not data for this entry yet
      free(data);
      data = NULL;
      // Search/copy list
      if (rc_list > 0) {
        rc_list = copyUntil(list, prefix, path, &l_line, &l_length, &status);
        if (rc_list < 0) {
          // Error copying list
          cerr << "Path search failed" << endl;
          rc = -1;
          break;
        }
      }
      // Copy journal path
      if (write(j_line, rc_journal) < 0) {
        // Could not write
        cerr << "Path write failed" << endl;
        rc = -1;
        break;
      }
    } else

    // Got data
    {
      // Must have a path before then
      if ((prefix == NULL) || (path == NULL)) {
        // Did not get anything before data
        cerr << "Data out of order in journal, line " << j_line_no << endl;
        errno = EUCLEAN;
        rc = -1;
        break;
      }
      free(data);
      data = NULL;
      asprintf(&data, "%s", j_line);
      // Copy journal data
      if (write(j_line, rc_journal) < 0) {
        // Could not write
        cerr << "Path write failed" << endl;
        rc = -1;
        break;
      }
    }
  }

  // Free resources and leave
  free(j_line);
  free(l_line);
  free(prefix);
  free(path);
  free(data);
  return rc;
}
