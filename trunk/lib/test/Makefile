AR := ar
RANLIB := ranlib
CXXFLAGS := -Wall -O2 -ansi -I..
LDFLAGS := -lssl -lz

all: test

test:   strings.done \
	files.done \
	filters.done \
	parsers.done \
	cvs_parser.done \
	dblist.done \
	db.done \
	paths.done \
	clients.done

clean:
	@rm -f *.[oa] *~ *.out *.err *.all *.done *_test zcop*
	@../../test_tools/test_setup clean

# Rules
%_test.o: %_test.cpp
	@echo "CXX	$<"
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

%_test: %_test.o ../libhbackup.a
	@echo "BUILD	$@"
	@$(CXX) $(LDFLAGS) -o $@ $^

%.done: %_test %.exp
	@echo "RUN	$<"
	@../../test_tools/test_run `basename $@ .done` && touch $@
