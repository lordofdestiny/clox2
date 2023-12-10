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

static int runFile(const char *path) {
    char *source = readFile(path);
    InterpretResult result = interpret(source);
    free(source);

    switch (result) {
    case INTERPRET_OK: return 0;
    case INTERPRET_EXIT: return vm.exit_code;
    case INTERPRET_COMPILE_ERROR: return 65;
    case INTERPRET_RUNTIME_ERROR: return 70;
    }
}

static int runBinaryFile(const char *path) {
    ObjFunction *compiled = loadBinary(path);
    InterpretResult result = interpretCompiled(compiled);

    switch (result) {
    case INTERPRET_EXIT: return vm.exit_code;
    case INTERPRET_RUNTIME_ERROR: return 70;
    default: return 0;
    }
}

static int compileFile(const char *src_path, const char *dest_path) {
    char *source = readFile(src_path);
    ObjFunction *code = compile(source);
    free(source);
    
    if (code == NULL) return INTERPRET_COMPILE_ERROR;
    writeBinary(code, dest_path);

    return 0;
}

int main(int argc, char *argv[]) {
    int exitCode = 0;
    clock_t start = clock();

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

    clock_t end = clock();
    printf("Execution time: %.6f seconds\n", ((float) (end - start)) / CLOCKS_PER_SEC);

    freeVM();

    exit(exitCode);
}
