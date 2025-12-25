#ifndef __CLOX2_ARGS_H__
#define __CLOX2_ARGS_H__

#include <argp.h>
#include <stdio.h>
#include <stdbool.h>

void printVersion(FILE *stream, struct argp_state *state);

typedef enum {
    CMD_REPL,
    CMD_EXECUTE,
    CMD_COMPILE,
    CMD_DISASSEMBLE,
} CommandType;

typedef struct {
    const char* input_file;
    const char* output_file;
    enum {
        CMD_EXEC_UNSET,
        CMD_EXEC_SOURCE,
        CMD_EXEC_BINARY,
    } input_type;
    enum {
        CMD_COMPILE_UNSET,
        CMD_COMPILE_BINARY,
        CMD_COMPILE_BYTECODE,
    } output_type;
    bool inline_code;
    CommandType type;
} Command;

#ifdef __cplusplus
extern "C" {
#endif

Command parseArgs(const int argc, char* argv[]);

#ifdef __cplusplus
}
#endif


#endif // __CLOX2_ARGS_H__