#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <cmocka.h>

#include "scanner.h"

#ifdef USE_FLEX_SCANNER

typedef struct {
    Scanner scanner;
    char* buffer;
    size_t size;
} ScannerWrapper;

void initScannerWrapper(ScannerWrapper* wrapper, const char* input) {
    size_t size = strlen(input) + 2;
    char* buffer = malloc(size);
    strcpy(buffer, input);
    buffer[size-1] = 0;
    buffer[size-2] = 0;

    wrapper->buffer = buffer;
    wrapper->size = size;
    initScanner(&wrapper->scanner, buffer, size);
}

void freeScannerWrapper(ScannerWrapper* wrapper) {
    freeScanner(&wrapper->scanner);
    free(wrapper->buffer);

    wrapper->buffer = NULL;
    wrapper->size = 0;
}

Token scan(ScannerWrapper* wrapper) {
    return scanToken(&wrapper->scanner);
}

#endif


#ifdef USE_FLEX_SCANNER

#define NEXT_TOKEN() scan(&wrapper)

#define PREPARE(text) \
    ScannerWrapper wrapper; \
    initScannerWrapper(&wrapper, text); \

#else

#define NEXT_TOKEN() scanToken()
#define PREPARE(text) initScanner(text)

#endif 


typedef struct {
    char * str;
    TokenType expected;
} TestData;

static void test_empty(void ** state) {
    PREPARE("");
    assert_int_equal(NEXT_TOKEN().type, TOKEN_EOF);
}
 
static void test_keywords(void **state) {
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
        tok = NEXT_TOKEN();
        assert_int_equal(tok.type, expected[i]);
    }
    tok = NEXT_TOKEN();
    assert_int_equal(tok.type, TOKEN_EOF);
}

static void test_symbols(void **state) {
    PREPARE("() [] {} + = += * *= **");
    const TokenType expected[] = {
        TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN, TOKEN_LEFT_BRACKET, TOKEN_RIGHT_BRACKET, 
        TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE, TOKEN_PLUS, TOKEN_EQUAL, TOKEN_PLUS_EQUAL,
        TOKEN_STAR, TOKEN_STAR_EQUAL, TOKEN_STAR_STAR
    };
    int expected_tokens = sizeof(expected) / sizeof(TokenType);
    Token tok;
    for(int i = 0; i < expected_tokens; i++) {
        tok = NEXT_TOKEN();
        assert_int_equal(tok.type, expected[i]);
    }
    tok = NEXT_TOKEN();
    assert_int_equal(tok.type, TOKEN_EOF);
}

static void test_non_keywords(void **state) {
    PREPARE("classic thorws asm quiro");
    Token tok;
    while(true) {
        tok = NEXT_TOKEN();
        if (tok.type == TOKEN_EOF){
            break;
        }
        assert_int_equal(tok.type, TOKEN_IDENTIFIER);
    }
}

static void test_number(void **state) {
    PREPARE("123 123.456");
    Token tok;
    while(true) {
        tok = NEXT_TOKEN();
        if (tok.type == TOKEN_EOF){
            break;
        }
        assert_int_equal(tok.type, TOKEN_NUMBER);
    }
}

static void test_string(void **state) {
    const char* str = "\"Hello world\"";
    PREPARE(str);
    Token tok;
    while(true) {
        tok = NEXT_TOKEN();
        if (tok.type == TOKEN_EOF){
            break;
        }
        assert_true(strncmp(str, tok.start, tok.length) == 0);
        assert_int_equal(tok.type, TOKEN_STRING);
    }
}

static void test_error(void **state) {
    PREPARE("\\ ^");
    Token tok;
    while(true) {
        tok = NEXT_TOKEN();
        if (tok.type == TOKEN_EOF){
            break;
        }
        assert_int_equal(tok.type, TOKEN_ERROR);
    }
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