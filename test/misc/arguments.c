#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <cmocka.h>

#include "args.h"

typedef struct {
    int argc;
    char** argv;
} MainArgs;

typedef struct {
    MainArgs* args;
    Command command;
} TestState;

MainArgs* makeMainArgs(int argc, ...) {
    MainArgs* args = malloc(sizeof(MainArgs));
    args->argc = argc;
    args->argv = calloc(argc+1, sizeof(char*));
    
    va_list vargs;
    va_start(vargs, argc);

    for(int i = 0; i < argc; i++) {
        char * arg = va_arg(vargs, char*);
        args->argv[i] = strdup(arg);
    }
    args->argv[args->argc] = NULL;

    va_end(vargs);

    return args;
}

void deleteMainArgs(MainArgs* args) {
    for(int i = 0; i < args->argc; i++) {
        free(args->argv[i]);
    }
    free(args->argv);
    free(args);
}

TestState* makeTestState(MainArgs* args, Command command) {
    TestState* test_state = malloc(sizeof(TestState));

    test_state->args = args;
    memcpy(&test_state->command, &command, sizeof(Command));

    return test_state;
}

void deleteTestState(TestState* state) {
    deleteMainArgs(state->args);
    free(state);
}

int teardownTest(void **state) {
    TestState* test_state = *(TestState**) state;
    deleteTestState(test_state);
    return 0;
}

void test(void** state) {
    TestState* test_state = *(TestState**) state;
    MainArgs* args = test_state->args;
    Command* cmd = &test_state->command;

    Command expected = parseArgs(args->argc, args->argv);
    if (expected.input_file != NULL) {
        assert_non_null_msg(expected.input_file, "Expected non-NULL input file");
        assert_string_equal(expected.input_file, cmd->input_file);
    }else {
        assert_ptr_equal(expected.input_file, NULL);
    }

    if (expected.output_file != NULL) {
        assert_non_null_msg(expected.output_file, "Expected non-NULL input file");
        assert_string_equal(expected.output_file, cmd->output_file);
    }else {
        assert_ptr_equal(expected.output_file, NULL);
    }

    assert_int_equal(expected.input_type, cmd->input_type);
    assert_int_equal(expected.output_type, cmd->output_type);
    assert_int_equal(expected.inline_code, cmd->inline_code);
    assert_int_equal(expected.type, cmd->type);
}

#define named_test(test_name, state) { \
    .name = test_name, \
    .test_func = test, \
    .setup_func = NULL, \
    .teardown_func = teardownTest, \
    .initial_state = state \
} \

int main(void) {    
    const struct CMUnitTest tests[] = {
        named_test("test_args_repl",
            makeTestState(
                makeMainArgs(1, "./clox"),
                (Command){
                    .input_file = NULL,
                    .output_file = NULL,
                    .input_type = CMD_EXEC_UNSET,
                    .output_type = CMD_COMPILE_UNSET,
                    .inline_code = false,
                    .type = CMD_REPL
                }
            )
        ),
        named_test("test_args_run_from_source",
            makeTestState(
                makeMainArgs(2, "./clox", "input.lox"),
                (Command){
                    .input_file = "input.lox",
                    .output_file = NULL,
                    .input_type = CMD_EXEC_SOURCE,
                    .output_type = CMD_COMPILE_UNSET,
                    .inline_code = false,
                    .type = CMD_EXECUTE
                }
            )
        ),
        named_test("test_args_run_from_binary",
            makeTestState(
                makeMainArgs(3, "./clox", "-xb", "input.lox.bin"),
                (Command) {
                    .input_file = "input.lox.bin",
                    .output_file = NULL,
                    .input_type = CMD_EXEC_BINARY,
                    .output_type = CMD_COMPILE_UNSET,
                    .inline_code = false,
                    .type = CMD_EXECUTE
                }
            )
        ),
        named_test("test_args_compile_source_to_binary",
            makeTestState(
                makeMainArgs(5, "./clox", "-c", "-o", "output.lox.bin", "input.lox"),
                (Command) {
                .input_file = "input.lox",
                .output_file = "output.lox.bin",
                .input_type = CMD_EXEC_SOURCE,
                .output_type = CMD_COMPILE_BINARY,
                .inline_code = false,
                .type = CMD_COMPILE
            }
            )
        ),
        named_test("test_compile_to_asm_inline_source",
            makeTestState(
                makeMainArgs(3, "./clox", "-is", "input.lox"),
                (Command) {
                .input_file = "input.lox",
                .output_file = NULL,
                .input_type = CMD_EXEC_SOURCE,
                .output_type = CMD_COMPILE_BYTECODE,
                .inline_code = true,
                .type = CMD_DISASSEMBLE
            }
            )
        ),
        named_test("test_disassemble_binary_to_bytecode",
            makeTestState(
                makeMainArgs(5, "./clox", "-sbi", "input.lox.bin","-o", "input.lox.s"),
                (Command) {
                .input_file = "input.lox.bin",
                .output_file = "input.lox.s",
                .input_type = CMD_EXEC_BINARY,
                .output_type = CMD_COMPILE_BYTECODE,
                .inline_code = true,
                .type = CMD_DISASSEMBLE
            }
            )
        )
    };
 
    return cmocka_run_group_tests(tests, NULL, NULL);
}
