BUILD_DIR=build

all: $(BUILD_DIR)/parser parser

CC=gcc

parser: build/parser
	ln -s $< $@

build/parser: build/parser_spec.o build/scanner_spec.o build/main.o
	@mkdir -p $(@D)
	gcc -ggdb -O0 -lm -o $@ $^

$(BUILD_DIR)/%.o: $(BUILD_DIR)/%.c
	@mkdir -p $(@D)
	gcc -ggdb -O0 -c -o $@ $< -Iinclude -I$(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(@D)
	gcc -ggdb -O0 -c -o $@ $< -Iinclude -I$(BUILD_DIR)

$(BUILD_DIR)/scanner_spec.c: scanner_spec.l
	@mkdir -p $(@D)
	flex --header-file=$(@:%.c=%.h) -o $@ $<

$(BUILD_DIR)/parser_spec.c: parser_spec.y
	@mkdir -p $(@D)
	bison -H$(@:%.c=%.h) -o$@ $< -Wcounterexamples

$(BUILD_DIR)/scanner_spec.h: $(BUILD_DIR)/scanner_spec.c
$(BUILD_DIR)/parser_spec.h: $(BUILD_DIR)/parser_spec.c

$(BUILD_DIR)/parser_spec.c: $(BUILD_DIR)/scanner_spec.h

clean:	
	rm -rf $(BUILD_DIR)
	rm -f parser
