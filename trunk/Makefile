MAJOR = 0
MINOR = 1
BUGFIX = 0

AR := ar
RANLIB := ranlib
STRIP := strip
CXXFLAGS := -Wall -O2 -ansi -pedantic
LDFLAGS := -lssl -lz
PREFIX := /usr/local/bin

all: hbackup

install: hbackup
	@echo "INSTALL	$<"
	@strip $^
	@mkdir -p ${PREFIX}
	@cp $^ ${PREFIX}

clean:
	@rm -f *.[oa] *~ version.h hbackup
	${MAKE} -C test clean

check: all
	@${MAKE} -C test

# Dependencies
libhbackup.a: clients.o cvs_parser.o db.o filters.o list.o paths.o files.o
version.h: libhbackup.a Makefile
hbackup: libhbackup.a version.h

clients.o: files.h list.h filters.h parser.h parsers.h cvs_parser.h paths.h \
	db.h clients.h
cvs_parser.o: files.h parser.h cvs_parser.h
db.o: list.h files.h filters.h parser.h parsers.h db.h
filters.o: filters.h
hbackup.o: list.h files.h db.h filters.h parser.h parsers.h cvs_parser.h \
	paths.h clients.h hbackup.h version.h
list.o: list.h
paths.o: files.h list.h filters.h parser.h parsers.h cvs_parser.h paths.h
files.o: files.h

# Rules
version.h:
	@echo "CREATE	$@"
	@echo "/* This file is auto-generated, do not edit. */" > version.h
	@echo "\n#ifndef VERSION_H\n#define VERSION_H\n" >> version.h
	@echo "#define VERSION_MAJOR $(MAJOR)" >> version.h
	@echo "#define VERSION_MINOR $(MINOR)" >> version.h
	@echo "#define VERSION_BUGFIX $(BUGFIX)" >> version.h
	@echo -n "#define BUILD " >> version.h
	@date -u +"%s" >> version.h
	@echo "\n#endif" >> version.h

%o: %cpp
	@echo "CXX	$<"
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

%.a:
	@echo "AR	$@"
	@$(AR) cru $@ $^
	@echo "RANLIB	$@"
	@$(RANLIB) $@

%: %.o
	@echo "BUILD	$@"
	@$(CXX) $(LDFLAGS) -o $@ $^
	@$(STRIP) $@
