AR := ar
CXXFLAGS := -Wall -O2 -ansi -pedantic -I..
LDFLAGS := -lssl -lz

all: test

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
	@rm -f *.[oa] *~ *.out *.done *_test
	@./test_setup clean

# Dependencies
list_test: ../list.a
filters_test: ../list.a
parsers_test: ../list.a
cvs_parser_test: ../list.a ../parsers.a
filelist_test: ../cvs_parser.a ../parsers.a ../filters.a ../list.a ../metadata.a \
	../params.a ../tools.a
db_test: ../filelist.a ../cvs_parser.a ../parsers.a ../filters.a ../list.a \
	../metadata.a ../params.a ../tools.a
clients_test: ../db.a ../filelist.a ../cvs_parser.a ../parsers.a ../filters.a \
	../list.a ../metadata.a ../params.a ../tools.a
hbackup: ../clients.a ../db.a ../filelist.a ../cvs_parser.a ../parsers.a \
	../filters.a ../list.a ../metadata.a ../params.a ../tools.a

# Rules
%_test.o: %_test.cpp ../%.a
	@echo "CXX	$<"
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

%.a: %.o
	@echo "AR	$^"
	@$(AR) cru $@ $^
	@echo "RANLIB	$@"
	@$(RANLIB) $@

%: %.o
	@echo "BUILD	$@"
	@$(CXX) $(LDFLAGS) -o $@ $^

%_test: %_test.o
	@echo "BUILD	$@"
	@$(CXX) $(LDFLAGS) -o $@ $^

%.done: %_test %.exp test_setup
	@echo "RUN	$<"
	@./test_setup
	@./$< > `basename $@ .done`.out 2>`basename $@ .done`.err
	@cat `basename $@ .done`.err `basename $@ .done`.out > `basename $@ .done`.all
	@diff -q `basename $@ .done`.exp `basename $@ .done`.all
	@touch $@
	@rm -f `basename $@ .done`.out `basename $@ .done`.err `basename $@ .done`.all
