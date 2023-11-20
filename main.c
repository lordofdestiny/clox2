#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "h/chunk.h"
#include "h/vm.h"
#include "h/compiler.h"
#include "h/binary.h"

static void repl() {
    char line[1024];

    while (true) {
        printf("> ");

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        interpret(line);
    }
}

static char *readFile(const char *path) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char *buffer = (char *) malloc(fileSize + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }
    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }
    buffer[bytesRead] = '\0';

    fclose(file);
    return buffer;
}

static void runFile(const char *path) {
    char *source = readFile(path);
    InterpretResult result = interpret(source);
    free(source);

    if (result == INTERPRET_COMPILE_ERROR) exit(65);
    if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}

static void runBinaryFile(const char *path) {
    ObjFunction *compiled = loadBinary(path);
    InterpretResult result = interpretCompiled(compiled);

    if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}

static void compileFile(const char *src_path, const char *dest_path) {
    char *source = readFile(src_path);
    ObjFunction *code = compile(source);
    free(source);
    writeBinary(code, dest_path);
}

int main(int argc, char *argv[]) {
    clock_t start = clock();
    initVM();

    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        runFile(argv[1]);
    } else if (argc == 3 && !strcmp(argv[1], "--bin")) {
        runBinaryFile(argv[2]);
    } else if (argc == 4 && !strcmp(argv[2], "--save")) {
        compileFile(argv[1], argv[3]);
    } else {
        fprintf(stderr, "Usage: clox [path] | [src_path --save dest_path] | [--bin bin_path]\n");
        exit(64);
    }

    freeVM();
    clock_t end = clock();

    printf("Execution time: %.6f seconds\n", ((float) (end - start)) / CLOCKS_PER_SEC);

    return 0;
}
