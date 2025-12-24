#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <setjmp.h>

#include "chunk.h"
#include "vm.h"
#include "compiler.h"
#include "binary.h"

#include "clox/args.h"
#include "clox/commands.h"

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

static int runFile(const char* path) {
    TextFile file = readTextFile(path, false);
    const InterpretResult result = interpret(file.content, false);
    freeTextFile(&file);

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
    TextFile source = readTextFile(src_path, false);
    ObjFunction* code = compile(source.content);
    freeTextFile(&source);

    if (code == NULL) return INTERPRET_COMPILE_ERROR;
    writeBinary(code, dest_path);

    return 0;
}

void executeCommand(const Command* cmd) {
    switch (cmd->type) {
    case CMD_REPL:
        repl();
        break;
    case CMD_EXECUTE:
        if(cmd->input_file == NULL) {
            fprintf(stderr, "No input file specified for execution.\n");
            exit(65);
        }
        if(cmd->input_type == CMD_EXEC_UNSET) {
            fprintf(stderr, "Input type not specified for execution.\n");
            exit(65);
        }
        if (cmd->input_type == CMD_EXEC_SOURCE) {
            runFile(cmd->input_file);
        } else if (cmd->input_type == CMD_EXEC_BINARY) {
            runBinaryFile(cmd->input_file);
        } else {
            fprintf(stderr, "Unknown input type for execution.\n");
            exit(65);
        }
        break;
    case CMD_COMPILE:
        if (cmd->input_file == NULL || cmd->output_file == NULL) {
            fprintf(stderr, "Input and output files must be specified for compilation.\n");
            exit(65);
        }
        if (cmd->input_type != CMD_EXEC_SOURCE) {
            fprintf(stderr, "Compilation only supported for source input.\n");
            exit(65);
        }
        if (cmd->output_type == CMD_COMPILE_UNSET) {
            fprintf(stderr, "Output type not specified for compilation.\n");
            exit(65);
        }
        compileFile(cmd->input_file, cmd->output_file);
        break;
    case CMD_DISASSEMBLE:
        fprintf(stderr, "Disassembly not implemented yet.\n");
        exit(65);
    default:
        fprintf(stderr, "Unsupported command type.\n");
        exit(65);
    }
}

int main(const int argc, char* argv[]) {
    int exitCode = 0;
    const clock_t start = clock();

    Command cmd = parseArgs(argc, argv);

    initVM();
    executeCommand(&cmd);

    const clock_t end = clock();
    printf("Execution time: %.6f seconds\n", ((float) (end - start)) / CLOCKS_PER_SEC);

    freeVM();
}
