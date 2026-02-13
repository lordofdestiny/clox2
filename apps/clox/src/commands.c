#include <time.h>
#include <stdlib.h>

#include <impl/binary.h>
#include <impl/compiler.h>
#include <impl/vm.h>

#include "exitcode.h"
#include "commands.h"

int repl() {
    setRepl(true);
    char line[1024];
    while (1) {
        memset(line, 0, sizeof(line));

        printf(">>> ");
        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        InputFile source = {.content = line, .size = strlen(line)};
        InterpretResult code = interpret(source);
        if (code == INTERPRET_EXIT) {
            return vmExitCode();
        }
    }

    return 0;
}

static void displayTime(clock_t start, clock_t end) {
    float time = ((float) (end - start)) / CLOCKS_PER_SEC;
    printf("Execution time: %.6f seconds\n", time);
}

static int handlerInputFileException(InputFileErrorCode code, const char* path) {
    int size = formatInputFileError(NULL, 0, path, code);

    char* errorBuffer = malloc(size);
    if (errorBuffer == NULL) {
        fprintf(stderr, "Failed to open file %s\n", path);
    }else {
        (void) formatInputFileError(errorBuffer, size, path, code);
        fprintf(stderr, "%s", errorBuffer);
    }
    
    return EXIT_CODE_FAILED_TO_READ_FILE;
}

static int runSourceFile(const char* path) {
    const clock_t start = clock();

    InputFile source;
    int ret = readInputFile(path, &source);
    if (ret != INPUT_FILE_SUCCESS) {
        return handlerInputFileException(ret, path);
    }
    InterpretResult result = interpret(source);

    const clock_t end = clock();
    displayTime(start, end);

    freeInputFile(&source);

    switch (result) {
    case INTERPRET_OK: return EXIT_SUCCESS;
    case INTERPRET_EXIT: return vmExitCode();
    case INTERPRET_COMPILE_ERROR: return EXIT_CODE_COMPILE_ERROR;
    case INTERPRET_RUNTIME_ERROR: return EXIT_CODE_RUNTIME_ERROR;
    }

    return 0;
}

static int runBinaryFile(const char* path) {
    clock_t start = clock();

    ObjFunction* compiled = loadBinary(path);
    InterpretResult result = interpretCompiled(compiled);
    
    clock_t end = clock();
    displayTime(start, end);

    switch (result) {
    case INTERPRET_EXIT: return vmExitCode();
    case INTERPRET_RUNTIME_ERROR: return EXIT_CODE_RUNTIME_ERROR;
    default: return 0;
    }
}

int runFile(const Command* cmd) {
    if (cmd->input_file == NULL) {
        fprintf(stderr, "No input file specified for execution.\n");
        return EXIT_CODE_BAD_ARGS;
    }
    
    if (cmd->input_type == CMD_EXEC_UNSET) {
        fprintf(stderr, "Input type not specified for execution.\n");
        return EXIT_CODE_BAD_ARGS;
    }

    if (cmd->input_type == CMD_EXEC_SOURCE) {
        return runSourceFile(cmd->input_file);
    }

    if (cmd->input_type == CMD_EXEC_BINARY) {
        return runBinaryFile(cmd->input_file);
    }

    fprintf(stderr, "Unknown input type for execution.\n");
    return EXIT_CODE_BAD_ARGS;
}

int compileFile(const Command* cmd) {
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

    InputFile source;
    readInputFile(cmd->input_file, &source);
    ObjFunction* bytecode = compile(source);
    freeInputFile(&source);

    int code = EXIT_SUCCESS;
    if (bytecode == NULL) {
        code = INTERPRET_COMPILE_ERROR;
    } else {
        writeBinary(cmd->input_file, bytecode, cmd->output_file);
    }

    clock_t end = clock();
    displayTime(start, end);

    return code;
}
