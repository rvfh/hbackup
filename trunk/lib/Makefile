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
libhbackup.a: clients.o db.o filters.o paths.o files.o cvs_parser.o

clients.o: files.h list.h filters.h parsers.h cvs_parser.h paths.h dbdata.h \
	db.h clients.h
cvs_parser.o: files.h parsers.h cvs_parser.h
db.o: list.h files.h filters.h parsers.h dbdata.h db.h
files.o: files.h
filters.o: filters.h
paths.o: files.h filters.h parsers.h cvs_parser.h paths.h

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
