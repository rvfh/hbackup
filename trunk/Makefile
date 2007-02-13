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
libhbackup.a: clients.o cvs_parser.o db.o filelist.o filters.o list.o \
	metadata.o tools.o
version.h: libhbackup.a Makefile
hbackup: libhbackup.a version.h

tools.o: hbackup.h tools.h
metadata.o: metadata.h
list.o: list.h
db.o: db.h list.h metadata.h tools.h hbackup.h
filters.o: filters.h list.h metadata.h
cvs_parser.o: cvs_parser.h parsers.h list.h metadata.h
filelist.o: filelist.h parsers.h list.h metadata.h tools.h hbackup.h
clients.o: clients.h list.h tools.h hbackup.h
hbackup.o: hbackup.h version.h

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
