LIBS=a b c d e

LIBS_FULL=$(LIBS:%=lib%.so)

all: libs main

main: main.c
	gcc -o $@ $< -ldl

libs: $(LIBS_FULL)

lib%.so: lib.c
	gcc -shared -fPIC -o $@ $< -DMESSAGE="\"lib $(@:lib%.so=%)\""

clean:
	rm -rf $(LIBS_FULL)
	rm -rf main

.PHONY: all libs clean
