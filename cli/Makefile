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
	@rm -f *.[oa] *~ hbackup version.h
	@${MAKE} -C test clean

check: all
	@${MAKE} -C test

# Dependencies
version.h: ../lib/libhbackup.a Makefile
hbackup: ../lib/libhbackup.a

hbackup.o: ../lib/hbackup.h version.h

# Rules
version.h:
	@echo "CREATE	$@"
	@echo "/* This file is auto-generated, do not edit. */" > version.h
	@echo >> version.h
	@echo "#ifndef VERSION_H" >> version.h
	@echo "#define VERSION_H" >> version.h
	@echo >> version.h
	@echo "#define VERSION_MAJOR $(MAJOR)" >> version.h
	@echo "#define VERSION_MINOR $(MINOR)" >> version.h
	@echo "#define VERSION_BUGFIX $(BUGFIX)" >> version.h
	@echo -n "#define BUILD " >> version.h
	@date -u +"%s" >> version.h
	@echo >> version.h
	@echo "#endif" >> version.h

%o: %cpp
	@echo "CXX	$<"
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

%: %.o
	@echo "BUILD	$@"
	@$(CXX) $(LDFLAGS) -o $@ $^
	@$(STRIP) $@
