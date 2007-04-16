MAJOR = 0
MINOR = 1
BUGFIX = 0

INCLUDES = ../lib

AR := ar
RANLIB := ranlib
STRIP := strip
CXXFLAGS := -Wall -O2 -ansi -I$(INCLUDES)
LDFLAGS := -lssl -lz
PREFIX := /usr/local/bin

all: hbackup

install: hbackup
	@echo "INSTALL	$<"
	@strip $^
	@mkdir -p ${PREFIX}
	@cp $^ ${PREFIX}

clean:
	@rm -f *.[oa] *~ hbackup
	@${MAKE} -C test clean

check: all
	@${MAKE} -C test

# Dependencies
hbackup: ../lib/libhbackup.a ../lib/version.h

hbackup.o: ../lib/list.h ../lib/files.h ../lib/db.h ../lib/filters.h \
	../lib/parsers.h ../lib/cvs_parser.h ../lib/paths.h ../lib/clients.h \
	../lib/hbackup.h ../lib/version.h

# Rules
%o: %cpp
	@echo "CXX	$<"
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

%: %.o
	@echo "BUILD	$@"
	@$(CXX) $(LDFLAGS) -o $@ $^
	@$(STRIP) $@
