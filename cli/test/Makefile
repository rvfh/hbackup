AR := ar
RANLIB := ranlib
CXXFLAGS := -Wall -O2 -ansi -I..
LDFLAGS := -lssl -lz

all: test

test:	hbackup.done

clean:
	@rm -f *.[oa] *~ *.out *.err *.all *.done *_test zcop*
	@../../test/test_setup clean

# Rules
hbackup.done: ../hbackup hbackup.exp ../../test/test_setup
	@echo "RUN	$<"
	@../../test/test_setup
	@rm -f hbackup.all
	@./$< -vvv -c etc/hbackup.conf >> hbackup.all 2>&1 || echo -n
	@./$< -vvv -c etc/hbackup.conf -C myhost -C client >> hbackup.all 2>&1 \
	|| echo -n
	@./$< -vvv -c etc/hbackup.conf -s >> hbackup.all 2>&1 || echo -n
	@./$< -vvv -c etc/hbackup.conf -t >> hbackup.all 2>&1 || echo -n
	@diff -q `basename $@ .done`.exp `basename $@ .done`.all
	@touch $@
	@rm -f `basename $@ .done`.all
