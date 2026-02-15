#include <stdlib.h>

#include <scanner/scanner_impl.h>

#include <scanner/scanner.h>
#include <scanner/token.h>

struct Scanner {
    yyscan_t yyscan;
	YY_BUFFER_STATE buffer;
};

int initScanner(Scanner** scanner_ptr, InputFile source) {
    yyscan_t yyscan;
    if(yylex_init(&yyscan) != 0) {
        return 1;
    }
    YY_BUFFER_STATE buffer = yy_scan_buffer(
        source.content,  source.size + 2,
        yyscan
    );
    
    if (buffer == NULL) {
        yylex_destroy(yyscan);
        return 2;
    }
    
    Scanner* scanner = malloc(sizeof(Scanner));
    if (scanner == NULL) {
        yy_delete_buffer(buffer, yyscan);
        yylex_destroy(yyscan);
    }
    scanner->yyscan = yyscan;
    scanner->buffer = buffer;

    yy_switch_to_buffer(buffer, yyscan);
    yyset_lineno(1, yyscan);
    yyset_column(1, yyscan);

    *scanner_ptr = scanner;

    return 0;
}

void freeScanner(Scanner* scanner) {
    yy_delete_buffer(scanner->buffer, scanner->yyscan);
    yylex_destroy( scanner ->yyscan);
    free(scanner);
}

Token scanToken(Scanner* scanner) {
    Token yylex(yyscan_t yyscanner);
    return yylex(scanner->yyscan);
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
