all: .timestamp

clean:
	-rm -f .timestamp

.timestamp: test_setup mount umount
	@echo "TOUCH	$@"
	@touch $@
