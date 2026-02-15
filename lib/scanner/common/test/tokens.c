
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define UNIT_TESTING 1
#include <cmocka.h>

#include <common/inputfile.h>

#include <scanner/scanner.h>

#define named_test(test_name, test_fn, state) { \
    .name = test_name, \
    .test_func = test_fn, \
    .setup_func = initTestState, \
    .teardown_func = freeTestState, \
    .initial_state = state, \
} \

typedef struct {
    InputFile file;
    Scanner* scanner;
    const char * str;
} TestStateBase;

typedef struct {
    TestStateBase common;
    TokenType expected;
} TestStateSingleType;

typedef struct {
    TestStateBase common;
    size_t count;
    TokenType* expected;
} TestStateMultiType;

static int initTestState(void** state) {
    TestStateBase* wrapper = *state;
    size_t size = strlen(wrapper->str);
    char* buffer = calloc(size + 2 ,1);
    strcpy(buffer, wrapper->str);

    wrapper->file = (InputFile) {
        .content = buffer,
        .path = NULL,
        .size = size
    };

    int succ = initScanner(&wrapper->scanner, wrapper->file);
    assert_int_equal(succ, 0);
    return 0;
}

static int freeTestState(void** state) {
    TestStateBase* wrapper = *state;
    freeInputFile(&wrapper->file);
    freeScanner(wrapper->scanner);
    free(*state);
    return 0;
}

static TestStateSingleType* makeSingleTypeData(const char* str, TokenType expected) {
    TestStateSingleType* data = malloc(sizeof(TestStateSingleType));
    data->common.str = str;
    data->expected = expected;
    return data;
}

static TestStateMultiType* makeMultipleTypeData(const char* str, size_t count, TokenType* expected) {
    TestStateMultiType* data = malloc(sizeof(TestStateMultiType));
    data->common.str = str;
    data->count = count;
    data->expected = expected;
    return data;
}

static void test_base_single_type(void** state) {
    TestStateSingleType* data = *state;

    while(true) {
        Token tok = scanToken(data->common.scanner);
        if (tok.type == TOKEN_EOF) break;
        assert_int_equal(tok.type, data->expected);
    }
}

static void test_base_multiple_types(void** state) {
    TestStateMultiType* data = *state;
    for(size_t i = 0; i < data->count; i++) {
        Token tok = scanToken(data->common.scanner);
        assert_int_equal(tok.type, data->expected[i]);
    }
    Token tok = scanToken(data->common.scanner);
    assert_int_equal(tok.type, TOKEN_EOF);
}

static void test_empty(void ** state) {
    TestStateSingleType* data = *state;
    assert_int_equal(scanToken(data->common.scanner).type, TOKEN_EOF);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        named_test("test_empty", test_empty,
            makeSingleTypeData("", TOKEN_EOF)
        ),

        named_test("test_keywords", test_base_multiple_types,
            makeMultipleTypeData(
                "and as break case catch "
                "class continue default else false "
                "for fun finally if nil "
                "or print return static super "
                "switch this throw true try "
                "var while",
                27,
                (TokenType*) &(TokenType[]) {
                    TOKEN_AND, TOKEN_AS, TOKEN_BREAK, TOKEN_CASE, TOKEN_CATCH,
                    TOKEN_CLASS, TOKEN_CONTINUE, TOKEN_DEFAULT, TOKEN_ELSE, TOKEN_FALSE,
                    TOKEN_FOR, TOKEN_FUN, TOKEN_FINALLY, TOKEN_IF, TOKEN_NIL,
                    TOKEN_OR, TOKEN_PRINT, TOKEN_RETURN, TOKEN_STATIC, TOKEN_SUPER,
                    TOKEN_SWITCH, TOKEN_THIS, TOKEN_THROW, TOKEN_TRUE, TOKEN_TRY,
                    TOKEN_VAR, TOKEN_WHILE
                }
            )
        ),
        named_test("test_symbols", test_base_multiple_types,
            makeMultipleTypeData(
                "() [] {} + = += * *= **",
                12,
                (TokenType*) &(TokenType[]) {
                    TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN, TOKEN_LEFT_BRACKET, TOKEN_RIGHT_BRACKET, 
                    TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE, TOKEN_PLUS, TOKEN_EQUAL, TOKEN_PLUS_EQUAL,
                    TOKEN_STAR, TOKEN_STAR_EQUAL, TOKEN_STAR_STAR
                }
            )
        ),
        named_test("test_string", test_base_multiple_types,
            makeMultipleTypeData(
                "\"Hello world\" \n"
                "\"\\a\\b\\t\\v\\f\\n\\r\" \n"
                "\"\\xab\" \n"
                "\"\\xfff\" \n"
                "\"\\141\" \n"
                "\"\\191\" \n"
                "\"\\141ab\" \n"
                "\"\\766\" \n", 
                8,
                (TokenType*) &(TokenType[]){
                    TOKEN_STRING, TOKEN_STRING, TOKEN_STRING,
                    TOKEN_ERROR, TOKEN_STRING, TOKEN_STRING,
                    TOKEN_STRING, TOKEN_ERROR 
                }
            )
        ),

        named_test(
            "test_non_keywords", test_base_single_type,
            makeSingleTypeData("classic thorws asm quiro",TOKEN_IDENTIFIER)
        ),
        // cmocka_unit_test(test_number),
        named_test(
            "test_number", test_base_single_type,
            makeSingleTypeData("123 123.456", TOKEN_NUMBER)
        ),
        named_test(
            "test_error", test_base_single_type,
            makeSingleTypeData("\\ ^", TOKEN_ERROR)
        )
    };
 
    return cmocka_run_group_tests(tests, NULL, NULL);
}
