AR := ar
RANLIB := ranlib
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
	clients.done \
	hbackup.done

clean:
	@rm -f *.[oa] *~ *.out *.err *.all *.done *_test zcop*
	@./test_setup clean

# Rules
%_test.o: %_test.cpp
	@echo "CXX	$<"
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

%_test: %_test.o ../libhbackup.a
	@echo "BUILD	$@"
	@$(CXX) $(LDFLAGS) -o $@ $^

%.done: %_test %.exp test_setup
	@echo "RUN	$<"
	@./test_setup
	@./$< > `basename $@ .done`.out 2>`basename $@ .done`.err
	@cat `basename $@ .done`.err `basename $@ .done`.out > `basename $@ .done`.all
	@rm -f `basename $@ .done`.err `basename $@ .done`.out
	@diff -q `basename $@ .done`.exp `basename $@ .done`.all
	@touch $@
	@rm -f `basename $@ .done`.all

hbackup.done: ../hbackup hbackup.exp test_setup
	@echo "RUN	$<"
	@./test_setup
	@./$< -vvv -c etc/hbackup.conf > hbackup.out 2>hbackup.err || echo -n
	@cat `basename $@ .done`.err `basename $@ .done`.out > `basename $@ .done`.all
	@rm -f `basename $@ .done`.err `basename $@ .done`.out
	@diff -q `basename $@ .done`.exp `basename $@ .done`.all
	@touch $@
	@rm -f `basename $@ .done`.all
