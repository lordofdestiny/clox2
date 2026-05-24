#include <time.h>
#include <stdlib.h>

#include <impl/binary.h>
#include <impl/compiler.h>
#include <impl/vm.h>

#include "exitcode.h"
#include "commands.h"

int repl() {
    char line[1024];
    while (true) {
        memset(line, 0, sizeof(line));

        fprintf(stdout, ">>> ");
        if (!fgets(line, sizeof(line), stdin)) {
            fprintf(stdout, "\n");
            break;
        }

        ObjFunction* script = compilePrompt(line);
        InterpretResult code = interpret(script);
        if (code == INTERPRET_EXIT) {
            return vmExitCode();
        }
    }

    return 0;
}

static void displayTime(clock_t start, clock_t end) {
    float time = ((float) (end - start)) / CLOCKS_PER_SEC;
    fprintf(stdout, "Execution time: %.6f seconds\n", time);
}

typedef ObjFunction* (*CreateFunction)(FILE* file);

static void displayFailedToOpenFileError(const char* path) {
    fprintf(stderr, "Could not open file \"%s\": %s\n", path, strerror(errno));
}

static int runFile(CreateFunction create, const char* path) {
    const clock_t start = clock();

    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        displayFailedToOpenFileError(path);
        return EXIT_CODE_FAILED_TO_READ_FILE;
    }

    ObjFunction* script = create(file);
    if (script == NULL) {
        fclose(file);
        return EXIT_CODE_COMPILE_ERROR;
    }
    InterpretResult code = interpret(script);
    fclose(file);

    const clock_t end = clock();
    displayTime(start, end);

    switch (code) {
    case INTERPRET_OK: return EXIT_SUCCESS;
    case INTERPRET_EXIT: return vmExitCode();
    case INTERPRET_COMPILE_ERROR: return EXIT_CODE_COMPILE_ERROR;
    case INTERPRET_RUNTIME_ERROR: return EXIT_CODE_RUNTIME_ERROR;
    }
    return 0;
}

int runFileCommand(const Command* cmd) {
    if (cmd->input_file == NULL) {
        fprintf(stderr, "No input file specified for execution.\n");
        return EXIT_CODE_BAD_ARGS;
    }
    
    if (cmd->input_type == CMD_EXEC_UNSET) {
        fprintf(stderr, "Input type not specified for execution.\n");
        return EXIT_CODE_BAD_ARGS;
    }

    if (cmd->input_type == CMD_EXEC_SOURCE) {
        return runFile(compileFile, cmd->input_file);
    }

    if (cmd->input_type == CMD_EXEC_BINARY) {
        return runFile(loadBinary, cmd->input_file);
    }

    fprintf(stderr, "Unknown input type for execution.\n");
    return EXIT_CODE_BAD_ARGS;
}

int compileFileCommand(const Command* cmd) {
    if (cmd->input_type != CMD_EXEC_SOURCE) {
        fprintf(stderr, "Compilation only supported for source input.\n");
        return EXIT_CODE_BAD_ARGS;
    }
    if (cmd->output_type == CMD_COMPILE_UNSET) {
        fprintf(stderr, "Output type not specified for compilation.\n");
        return EXIT_CODE_BAD_ARGS;
    }
    
    if (cmd->input_file == NULL || cmd->output_file == NULL) {
        fprintf(stderr, "Input and output files must be specified for compilation.\n");
        return EXIT_CODE_BAD_ARGS;
    }

    clock_t start = clock();

    FILE* file = fopen(cmd->input_file, "rb");
    if (file == NULL) {
        displayFailedToOpenFileError(cmd->input_file);
        return EXIT_CODE_FAILED_TO_READ_FILE;
    }

    ObjFunction* bytecode = compileFile(file);
    fclose(file);

    int code = EXIT_SUCCESS;
    if (bytecode == NULL) {
        code = INTERPRET_COMPILE_ERROR;
    } else {
        writeBinary(cmd->output_file, cmd->input_file, bytecode);
    }

    clock_t end = clock();
    displayTime(start, end);

    return code;
}
