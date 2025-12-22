#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <setjmp.h>

#include "chunk.h"
#include "vm.h"
#include "compiler.h"
#include "binary.h"

// char ** file_lines;
// int lines = 1;

jmp_buf exit_repl;

static void repl() {
    while (1) {
        char line[1024];
        printf("> ");

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        if(setjmp(exit_repl) == 0) {
            interpret(line, true);
        } else {
            exit(vm.exit_code);
        }
    }
}

static char* readFile(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }

    fseek(file, 0L, SEEK_END);
    const size_t fileSize = ftell(file);
    rewind(file);

    char* buffer = (char*) malloc(fileSize + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }
    const size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }
    buffer[bytesRead] = '\0';

    fclose(file);
    return buffer;
}

static int runFile(const char* path) {
    char* source = readFile(path);
    const InterpretResult result = interpret(source, false);
    free(source);

    switch (result) {
    case INTERPRET_OK: return 0;
    case INTERPRET_EXIT: return vm.exit_code;
    case INTERPRET_COMPILE_ERROR: return 65;
    case INTERPRET_RUNTIME_ERROR: return 70;
    }

    return 0;
}

static int runBinaryFile(const char* path) {
    ObjFunction* compiled = loadBinary(path);
    const InterpretResult result = interpretCompiled(compiled);

    switch (result) {
    case INTERPRET_EXIT: return vm.exit_code;
    case INTERPRET_RUNTIME_ERROR: return 70;
    default: return 0;
    }
}

static int compileFile(const char* src_path, const char* dest_path) {
    char* source = readFile(src_path);
    ObjFunction* code = compile(source);
    free(source);

    if (code == NULL) return INTERPRET_COMPILE_ERROR;
    writeBinary(code, dest_path);

    return 0;
}

int main(const int argc, char* argv[]) {
    int exitCode = 0;
    const clock_t start = clock();

    initVM();

    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        exitCode = runFile(argv[1]);
    } else if (argc == 3 && !strcmp(argv[2], "--bin")) {
        exitCode = runBinaryFile(argv[1]);
    } else if (argc == 4 && !strcmp(argv[2], "--save")) {
        exitCode = compileFile(argv[1], argv[3]);
    } else {
        exitCode = 65;
        fprintf(stderr, "Usage: clox [path] | [src_path --save dest_path] | [--bin bin_path]\n");
    }

    const clock_t end = clock();
    printf("Execution time: %.6f seconds\n", ((float) (end - start)) / CLOCKS_PER_SEC);

    freeVM();

    exit(exitCode);
}
