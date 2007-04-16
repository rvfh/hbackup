all:
	@$(MAKE) -C lib all
	@$(MAKE) -C cli all

install:
	@$(MAKE) -C lib install
	@$(MAKE) -C cli install

clean:
	@$(MAKE) -C lib clean
	@$(MAKE) -C cli clean

check:
	@$(MAKE) -C lib check
	@$(MAKE) -C cli check
