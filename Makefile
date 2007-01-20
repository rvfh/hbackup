MAJOR = 0
MINOR = 1
BUGFIX = 0

AR := ar
CC := gcc
RANLIB := ranlib
CFLAGS := -Wall -O2 -ansi -pedantic
LDFLAGS := -lssl -lz
PREFIX := /usr/local/bin

all: test hbackup

install: hbackup
	@echo "INSTALL	$<"
	@strip $^
	@mkdir -p ${PREFIX}
	@cp $^ ${PREFIX}

test:	params.done \
	list.done \
	tools.done \
	metadata.done \
	filters.done \
	parsers.done \
	cvs_parser.done \
	filelist.done \
	db.done \
	clients.done

clean:
	@rm -f *.[oa] *~ *.out *.done *_test version.h hbackup
	@./test_setup clean

# Dependencies
list_test: list.a
filters_test: list.a
parsers_test: list.a
cvs_parser_test: list.a parsers.a
filelist_test: cvs_parser.a parsers.a filters.a list.a metadata.a params.a \
	tools.a
db_test: filelist.a cvs_parser.a parsers.a filters.a list.a metadata.a \
	params.a tools.a
clients_test: db.a filelist.a cvs_parser.a parsers.a filters.a list.a \
	metadata.a params.a tools.a
hbackup: clients.a db.a filelist.a cvs_parser.a parsers.a filters.a list.a \
	metadata.a params.a tools.a

tools.o: hbackup.h tools.h
metadata.o: metadata.h
list.o: list.h
db.o: db.h list.h metadata.h tools.h hbackup.h
filters.o: filters.h list.h
parsers.o: parsers.h list.h
cvs_parser.o: cvs_parser.h parsers.h list.h metadata.h
filelist.o: filelist.h parsers.h list.h metadata.h tools.h hbackup.h
clients.o: clients.h list.h tools.h hbackup.h
hbackup.o: hbackup.h version.h

# Rules
version.h: Makefile
	@echo "CREATE	$@"
	@echo "/* This file is auto-generated, do not edit. */" > version.h
	@echo "\n#ifndef VERSION_H\n#define VERSION_H\n" >> version.h
	@echo "#define VERSION_MAJOR $(MAJOR)" >> version.h
	@echo "#define VERSION_MINOR $(MINOR)" >> version.h
	@echo "#define VERSION_BUGFIX $(BUGFIX)" >> version.h
	@echo -n "#define BUILD " >> version.h
	@if [ -d .svn ]; then \
	  svn info | grep "Last Changed Rev:" | sed "s/.*: *//" >> version.h; \
	else \
	  echo "0" >> version.h; \
	fi
	@echo "\n#endif" >> version.h

%o: %c
	@echo "CC	$<"
	@$(CC) $(CFLAGS) -c -o $@ $<

%.a: %.o
	@echo "AR	$^"
	@$(AR) cru $@ $^
	@echo "RANLIB	$@"
	@$(RANLIB) $@

%: %.o
	@echo "BUILD	$@"
	@$(CC) $(LDFLAGS) -o $@ $^

%_test: %_test.o
	@echo "BUILD	$@"
	@$(CC) $(LDFLAGS) -o $@ $^

%.done: %_test %.exp test_setup
	@echo "RUN	$<"
	@./test_setup
	@./$< > `basename $@ .done`.out 2>`basename $@ .done`.err
	@cat `basename $@ .done`.err `basename $@ .done`.out > `basename $@ .done`.all
	@diff -q `basename $@ .done`.exp `basename $@ .done`.all
	@touch $@
	@rm -f `basename $@ .done`.out `basename $@ .done`.err `basename $@ .done`.all
