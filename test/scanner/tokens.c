
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define UNIT_TESTING 1
#include <cmocka.h>

#include "scanner.h"
#include "inputfile.h"

typedef struct {
    Scanner scanner;
    InputFile file;
} TestState;

void initTestState(TestState* wrapper, const char* input) {
    size_t size = strlen(input) ;
    char* buffer = calloc(size + 2 ,1);
    strcpy(buffer, input);

    wrapper->file = (InputFile) {
        .content = buffer,
        .path = NULL,
        .size = size
    };
    initScanner(&wrapper->scanner, wrapper->file);
}

void freeTestState(TestState* wrapper) {
    freeScanner(&wrapper->scanner);
    freeInputFile(&wrapper->file);
}

#define PREPARE(text) \
    TestState wrapper; \
    initTestState(&wrapper, text); \

typedef struct {
    char * str;
    TokenType expected;
} TestData;

static void test_empty(void **) {
    PREPARE("");
    assert_int_equal(scanToken(&wrapper.scanner).type, TOKEN_EOF);
}
 
static void test_keywords(void **) {
    PREPARE(
        "and as break case catch "
        "class continue default else false "
        "for fun finally if nil "
        "or print return static super "
        "switch this throw true try "
        "var while"
    );

    const TokenType expected[] = {
        TOKEN_AND, TOKEN_AS, TOKEN_BREAK, TOKEN_CASE, TOKEN_CATCH,
        TOKEN_CLASS, TOKEN_CONTINUE, TOKEN_DEFAULT, TOKEN_ELSE, TOKEN_FALSE,
        TOKEN_FOR, TOKEN_FUN, TOKEN_FINALLY, TOKEN_IF, TOKEN_NIL,
        TOKEN_OR, TOKEN_PRINT, TOKEN_RETURN, TOKEN_STATIC, TOKEN_SUPER,
        TOKEN_SWITCH, TOKEN_THIS, TOKEN_THROW, TOKEN_TRUE, TOKEN_TRY,
        TOKEN_VAR, TOKEN_WHILE
    };
    int expected_tokens = sizeof(expected) / sizeof(TokenType);
    Token tok;
    for(int i = 0; i < expected_tokens; i++) {
        tok = scanToken(&wrapper.scanner);
        assert_int_equal(tok.type, expected[i]);
    }
    tok = scanToken(&wrapper.scanner);
    assert_int_equal(tok.type, TOKEN_EOF);
    freeTestState(&wrapper);
}

static void test_symbols(void **) {
    PREPARE("() [] {} + = += * *= **");
    const TokenType expected[] = {
        TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN, TOKEN_LEFT_BRACKET, TOKEN_RIGHT_BRACKET, 
        TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE, TOKEN_PLUS, TOKEN_EQUAL, TOKEN_PLUS_EQUAL,
        TOKEN_STAR, TOKEN_STAR_EQUAL, TOKEN_STAR_STAR
    };
    int expected_tokens = sizeof(expected) / sizeof(expected[0]);
    Token tok;
    for(int i = 0; i < expected_tokens; i++) {
        tok = scanToken(&wrapper.scanner);
        assert_int_equal(tok.type, expected[i]);
    }
    tok = scanToken(&wrapper.scanner);
    assert_int_equal(tok.type, TOKEN_EOF);
    freeTestState(&wrapper);
}

static void test_non_keywords(void **) {
    PREPARE("classic thorws asm quiro");
    Token tok;
    while(true) {
        tok = scanToken(&wrapper.scanner);
        if (tok.type == TOKEN_EOF){
            break;
        }
        assert_int_equal(tok.type, TOKEN_IDENTIFIER);
    }
    freeTestState(&wrapper);
}

static void test_number(void **) {
    PREPARE("123 123.456");
    Token tok;
    while(true) {
        tok = scanToken(&wrapper.scanner);
        if (tok.type == TOKEN_EOF){
            break;
        }
        assert_int_equal(tok.type, TOKEN_NUMBER);
    }
    freeTestState(&wrapper);
}

static void test_string(void **) {
    struct STest {
        char * input;
        TokenType expectedType;
    };

    struct STest tests[] = {
        { "\"Hello world\"", TOKEN_STRING },
        { "\"\\a\\b\\t\\v\\f\\n\\r\"", TOKEN_STRING },
        { "\"\\xab\"", TOKEN_STRING },
        { "\"\\xfff\"", TOKEN_ERROR },
        { "\"\\141\"", TOKEN_STRING },
        { "\"\\191\"", TOKEN_STRING },
        { "\"\\141ab\"", TOKEN_STRING },
        { "\"\\766\"", TOKEN_ERROR },
    };
    int stestCount = sizeof(tests) / sizeof(tests[0]);
    for (int i = 0; i < stestCount; i++) {
        PREPARE(tests[i].input);
        Token tok;
        while(true) {
            tok = scanToken(&wrapper.scanner);
            if (tok.type == TOKEN_EOF){
                break;
            }
            printf("%s -> %.*s\n", tests[i].input, tok.length, tok.start);
            assert_int_equal(tok.type, tests[i].expectedType);
        }
        freeTestState(&wrapper);
    }
}

static void test_error(void **) {
    PREPARE("\\ ^");
    Token tok;
    while(true) {
        tok = scanToken(&wrapper.scanner);
        if (tok.type == TOKEN_EOF){
            break;
        }
        assert_int_equal(tok.type, TOKEN_ERROR);
    }
    freeTestState(&wrapper);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_empty),
        cmocka_unit_test(test_symbols),
        cmocka_unit_test(test_keywords),
        cmocka_unit_test(test_non_keywords),
        cmocka_unit_test(test_number),
        cmocka_unit_test(test_string),
        cmocka_unit_test(test_error),
    };
 
    return cmocka_run_group_tests(tests, NULL, NULL);
}
