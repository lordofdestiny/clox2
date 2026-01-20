default:
	$(error You need to provide a target)

docs:
	$(MAKE) -C docs all
	
clean:
	$(MAKE) -C docs $@

.PHONY: default docs clean
