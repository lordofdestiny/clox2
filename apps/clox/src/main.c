#include <stdio.h>
#include <stdlib.h>

#include <args.h>
#include "commands.h"
#include "exitcode.h"
#include <impl/vm.h>

int executeCommand(const Command* cmd) {
    switch (cmd->type) {
    case CMD_REPL:
        return repl();
    case CMD_EXECUTE:
        return runFile(cmd);
    case CMD_COMPILE:
        return compileFile(cmd);
    case CMD_DISASSEMBLE:
        fprintf(stderr, "Disassembly not implemented yet.\n");
        return EXIT_CODE_BAD_ARGS;
    default:
        fprintf(stderr, "Unsupported command type.\n");
        return EXIT_CODE_BAD_ARGS;
    }
}

int main(const int argc, char* argv[]) {
    Command cmd = parseArgs(argc, argv);
    
    initVM();
    int exitCode = executeCommand(&cmd);
    freeVM();

    exit(exitCode);
}
