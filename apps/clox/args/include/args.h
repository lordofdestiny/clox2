#ifndef __CLOX2_ARGS_H__
#define __CLOX2_ARGS_H__

#include <argp.h>
#include <stdbool.h>

typedef enum {
    CMD_NONE,
    CMD_REPL,
    CMD_EXECUTE,
    CMD_COMPILE,
    CMD_DISASSEMBLE,
} CommandType;

typedef enum {
    CMD_EXEC_UNSET,
    CMD_EXEC_SOURCE,
    CMD_EXEC_BINARY,
} CommandInputType;

typedef enum {
    CMD_COMPILE_UNSET,
    CMD_COMPILE_BINARY,
    CMD_COMPILE_BYTECODE,
} CommandOutputType;

typedef struct {
    const char* input_file;
    const char* output_file;
    CommandInputType input_type;
    CommandOutputType output_type;
    CommandType type;
    bool inline_code;
} Command;

Command parseArgs(const int argc, char* argv[]);

#endif // __CLOX2_ARGS_H__
