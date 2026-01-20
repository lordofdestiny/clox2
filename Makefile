default:
	$(error You need to provide a target)
	
%:
	$(MAKE) -C parser_bison $@

clean:
	$(MAKE) -C docs $@
	$(MAKE) -C parser_bison $@

.PHONY: Makefile default docs
