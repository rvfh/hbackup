AR := ar
RANLIB := ranlib
CXXFLAGS := -Wall -O2 -ansi -I..
LDFLAGS := -lssl -lz

all: test

test:	hbackup.done

clean:
	@rm -f *.[oa] *~ *.out *.err *.all *.done *_test zcop*
	@../../test_setup clean

# Rules
hbackup.done: ../hbackup hbackup.exp ../../test_setup
	@echo "RUN	$<"
	@../../test_setup
	@./$< -vvv -c etc/hbackup.conf >hbackup.all 2>&1 || echo -n
	@./$< -vvv -c etc/hbackup.conf -s >>hbackup.all 2>&1 || echo -n
	@./$< -vvv -c etc/hbackup.conf -t >>hbackup.all 2>&1 || echo -n
	@diff -q `basename $@ .done`.exp `basename $@ .done`.all
	@touch $@
	@rm -f `basename $@ .done`.all
