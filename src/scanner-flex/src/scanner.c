#include <stdlib.h>

#include "scanner_impl.h"

#include "scanner.h"
#include "token.h"

typedef struct {
	YY_BUFFER_STATE buff;
    yyscan_t yyscan;
} ScannerImpl;

void initScanner(Scanner* scanner, InputFile source) {
    scanner->impl = calloc(1, sizeof(ScannerImpl));
    ScannerImpl* impl = scanner->impl;

    yylex_init(&impl->yyscan);
	impl->buff = yy_scan_buffer(source.content,  source.size + 2, impl->yyscan);
	yy_switch_to_buffer(impl->buff, impl->yyscan);
    yyset_lineno(1, impl->yyscan);
    yyset_column(1, impl->yyscan);
}

void freeScanner(Scanner* scanner) {
    ScannerImpl* impl = scanner->impl;

    yy_delete_buffer(impl->buff, impl->yyscan);
    yylex_destroy( impl ->yyscan);

    free(impl);
    scanner->impl = NULL;
}

Token scanToken(Scanner* scanner) {
    ScannerImpl* impl = scanner->impl;

    Token yylex(yyscan_t yyscanner);
    return yylex(impl->yyscan);
}

static TokenLocation getLocation(yyscan_t yyscanner) {
    return (TokenLocation) {
        .line = yyget_lineno(yyscanner),
        .column = yyget_column(yyscanner)
    };
}

Token makeToken(yyscan_t yyscanner, const TokenType type) {
    return (Token) {
        .loc = getLocation(yyscanner),
        .type = type,
        .length = yyget_leng(yyscanner),
        .start = yyget_text(yyscanner),
    };
}

Token errorToken(yyscan_t yyscanner, const char* message) {
    return (Token) {
        .loc = getLocation(yyscanner),
        .type = TOKEN_ERROR,
        .length = strlen(message),
        .start = message,
    };
}
