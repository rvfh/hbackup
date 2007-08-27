AR := ar
RANLIB := ranlib
STRIP := strip
CXXFLAGS := -Wall -O2 -ansi
LDFLAGS := -lssl -lz
PREFIX := /usr/local

all: libhbackup.a

install: all

clean:
	@rm -f *.[oa] *~
	@${MAKE} -C test clean

check: all
	@${MAKE} -C test

# Dependencies
libhbackup.a: interface.o clients.o db.o dblist.o filters.o paths.o files.o \
	cvs_parser.o strings.o

clients.o: files.h filters.h parsers.h cvs_parser.h paths.h dbdata.h \
	dblist.h db.h clients.h
cvs_parser.o: files.h parsers.h cvs_parser.h
db.o: files.h dbdata.h dblist.h db.h
dblist.o: files.h dbdata.h dblist.h
strings.o: strings.h
files.o: files.h
filters.o: filters.h files.h
paths.o: files.h filters.h parsers.h cvs_parser.h dbdata.h dblist.h db.h paths.h
interface.o: hbackup.h clients.h db.h

# Rules
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
