AR := ar
RANLIB := ranlib
CXXFLAGS := -Wall -O2 -ansi -I..
LDFLAGS := -lssl -lz

all: test

test:	hbackup.done

clean:
	@rm -f *.[oa] *~ *.out *.err *.all *.done *_test zcop*
	@../../test_tools/test_setup clean

# Rules
hbackup.done: ../hbackup hbackup.exp
	@echo "RUN	$<"
	@../../test_tools/test_setup
	@rm -f hbackup.all
	@PATH=../../test_tools:$(PATH) ./$< -vvv -c etc/hbackup.conf >> hbackup.all 2>&1 || echo -n
	@PATH=../../test_tools:$(PATH) ./$< -vvv -c etc/hbackup.conf -C myhost -C client >> hbackup.all 2>&1 \
	|| echo -n
	@PATH=../../test_tools:$(PATH) ./$< -vvv -c etc/hbackup.conf -s >> hbackup.all 2>&1 || echo -n
	@PATH=../../test_tools:$(PATH) ./$< -vvv -c etc/hbackup.conf -t >> hbackup.all 2>&1 || echo -n
	@diff -q `basename $@ .done`.exp `basename $@ .done`.all
	@touch $@
	@rm -f `basename $@ .done`.all
