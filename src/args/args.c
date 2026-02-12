#include <argp.h>
#include <complex.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>

#include "args.h"

static void printVersion(FILE *stream, struct argp_state *state) {
    (void) state;
    fprintf(stream, "%s\n", argp_program_version);
}

#define SetInputType(state, options, option)            \
    if (options->input_type == IN_UNSET) {              \
        options->input_type = IN_##option;              \
    }else {                                             \
        argp_error (state, "Input type already set. Only one of -l or -b allowed.");   \
    }

#define SetOutputType(state, options, option)           \
    if (options->output_type == OUT_UNSET) {            \
        options->output_type = OUT_##option;            \
    }else {                                             \
        argp_error (state, "Output type already set. Only one of -x, -c or -s allowed.");  \
    }

typedef struct{
    char* input_file;
    char* output_file;
    enum {
        IN_UNSET,
        IN_SOURCE,
        IN_BINARY,
    } input_type;
    enum {
        OUT_UNSET,
        OUT_EXECUTE,
        OUT_BINARY,
        OUT_BYTECODE,
    } output_type;
    bool inline_code;
} ParsingOptions;

static error_t argpParser (int key, char *arg, struct argp_state *state) {
    ParsingOptions* options = state->input;

    switch(key) {
        case 'x':
            SetOutputType(state, options, EXECUTE);
            break;
        case 'c':
            SetOutputType(state, options, BINARY);
            break;
        case 's':
            SetOutputType(state, options, BYTECODE);
            break;
        case 'l':
            SetInputType(state, options, SOURCE);
            break;
        case 'b':
            SetInputType(state, options, BINARY);
            break;
        case 'o':
            options->output_file=arg;
            break;
        case 'i':
            options->inline_code = true;
            break;
        case ARGP_KEY_ARG:
            if (state->arg_num == 0)
                options->input_file = arg;
            else
                return ARGP_ERR_UNKNOWN;
            break;
        case ARGP_KEY_ARGS:{
            char** remaining_args = state->argv + state->next;
            (void) remaining_args;
            int num_remaining_args = state->argc - state->next;
            if (num_remaining_args > 0) {
                argp_error(state, "Excessive positional arguments. Only one argument allowed");
            }
            break;
        }
        case ARGP_KEY_END:
            if (options->inline_code && options->output_type != OUT_BYTECODE) {
                argp_error (state, "Inline code can only be used with bytecode output.");
            }
            if (options->input_type == IN_UNSET) {
                options->input_type = IN_SOURCE;
            }

            if (options->output_type == OUT_UNSET && options->input_file != NULL) {
                options->output_type = OUT_EXECUTE;
            }
            if ((options->output_type == OUT_EXECUTE || options->output_type == OUT_BINARY )
                && options->input_file == NULL) {
                argp_error (state, "No input file specified.");
            }
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

Command parseArgs(const int argc, char *argv[]) {
    argp_program_version = "clox2 v1.0.0";
    argp_program_version_hook = &printVersion;

    struct argp state = {
        .args_doc = "[FILE]",
        .options = (struct argp_option*)&(struct argp_option[]){
            {
                .doc="Input type options:",
            },
            {
                .key='x',
                .doc="Execute the input file (default)",
            },
            {
                .key='c',
                .doc="Compile file into binary",
            },
            {
                .key='s',
                .doc="Compile file into bytecode",
            },
            {
                .doc="Output type options:",
            },
            {
                .key='l',
                .doc="Threat the input file as source file (default)",
            },
            {
                .key='b',
                .doc="Treat input file is a binary file",
            },
            {.doc="Output options:"},
            {
                .key='o',
                .arg="filename",
                .doc="Output file path",
            },
            {
                .key='i',
                .doc="Inline code in bytecode output",
            },
            {
                .name = NULL,
                .key = 0,
                .arg = NULL,
                .flags = 0,
                .doc = 0,
                .group = 0
            }
        },
        .parser = &argpParser,
    };
    
    int parsedArgs = 0;
    ParsingOptions options = {
        .input_file = NULL,
        .output_file = NULL,
        .input_type = IN_UNSET,
        .output_type = OUT_UNSET
    };
    argp_parse (&state, argc, argv,ARGP_IN_ORDER, &parsedArgs, &options);

    if (parsedArgs <= 1) {
        return (Command){
            .type = CMD_REPL,
        };
    }

    switch(options.output_type ) {
        case OUT_UNSET:
        case OUT_EXECUTE:
            return (Command) {
                .type = CMD_EXECUTE,
                .input_file = options.input_file,
                .input_type = (options.input_type == IN_SOURCE) 
                                ? CMD_EXEC_SOURCE
                                : CMD_EXEC_BINARY,
            };
        case OUT_BINARY:
            return (Command){
                .type = CMD_COMPILE,
                .input_file = options.input_file,
                .output_file = options.output_file,
                .inline_code = options.inline_code,
                .input_type = (options.input_type == IN_SOURCE) 
                                ? CMD_EXEC_SOURCE
                                : CMD_EXEC_BINARY,
                .output_type = (options.output_type == OUT_BINARY) 
                                ? CMD_COMPILE_BINARY
                                : CMD_COMPILE_BYTECODE,
            };
        case OUT_BYTECODE:
            return (Command){
                .type = CMD_DISASSEMBLE,
                .input_file = options.input_file,
                .output_file = options.output_file,
                .input_type = (options.input_type == IN_SOURCE) 
                                ? CMD_EXEC_SOURCE
                                : CMD_EXEC_BINARY,
                .output_type = (options.output_type == OUT_BINARY) 
                                ? CMD_COMPILE_BINARY
                                : CMD_COMPILE_BYTECODE,
                .inline_code = options.inline_code,
            };
        default:
            return (Command){
                .type = CMD_NONE
            };
    };
}
