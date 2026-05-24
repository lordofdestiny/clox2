#include <stdlib.h>

#include <scanner/gen/scanner_impl.h>

#include <scanner/scanner.h>
#include <scanner/token.h>

enum ScannerErrorCode : int {
    SCANNER_ERROR_SCANNER_DEST_NULL = 1,
    SCANNER_ERROR_FILE_NULL,
    SCANNER_ERROR_FILE_OPEN_FAILED,
    SCANNER_ERROR_ALLOC_FAILED,
    SCANNER_YY_INIT_FAILED,
    SCANNER_YY_CREATE_BUFFER_FAILED,
    SCANNER_ERROR_LAST = SCANNER_YY_CREATE_BUFFER_FAILED
};

struct Scanner {
    yyscan_t yyscan;
	YY_BUFFER_STATE buffer;
    FILE* source;
};

typedef  xerror*(*CreateBufferType)(Scanner*, yyscan_t, const char*);

static xerror* initScannerImpl(Scanner** scanner_ptr, const char* text, CreateBufferType createBuffer) {
    if (scanner_ptr == NULL) {
        return XERROR("clox_scanner", SCANNER_ERROR_SCANNER_DEST_NULL, "Scanner destination is null");
    }

    if (text == NULL) {
        return XERROR("clox_scanner", SCANNER_ERROR_FILE_NULL, "File path is null");
    }

    Scanner* scanner = malloc(sizeof(Scanner));
    if(scanner == NULL) {
        return XERROR_LIBC("Failed to allocate scanner");
    }

    yyscan_t yyscan;
    if(yylex_init(&yyscan) != 0) {
        free(scanner);
        return XERROR("clox_scanner", SCANNER_YY_INIT_FAILED, "Failed to initialize flex scanner");
    }
    
    xerror* err = createBuffer(scanner, yyscan, text);
    if (err != NULL)  {
        scanner->yyscan = NULL;
        scanner->buffer = NULL;
        scanner->source = NULL;
        return err;
    }

    yy_switch_to_buffer(scanner->buffer, yyscan);
    yyset_lineno(1, yyscan);
    yyset_column(1, yyscan);

    *scanner_ptr = scanner;

    return NULL;
}


static xerror* createFileBuffer(Scanner* scanner, yyscan_t yyscan, const char* path) {
    scanner->yyscan = yyscan;

    FILE* file = fopen(path, "r");
    if (file == NULL) {
        free(scanner);
        yylex_destroy(yyscan);
        return XERROR_LIBC("Failed to open input file");
    }
    scanner->source = file;
    
    YY_BUFFER_STATE buffer = yy_create_buffer(file,YY_BUF_SIZE, yyscan);
    if(buffer == NULL) {
        fclose(file);
        free(scanner);
        yylex_destroy(yyscan);
        return XERROR("clox_scanner", SCANNER_YY_CREATE_BUFFER_FAILED, "Failed to create scanner buffer");
    }
    scanner->buffer = buffer;

    return 0;
}

static xerror* createStringBuffer(Scanner* scanner, yyscan_t yyscan, const char* prompt) {
    scanner->yyscan = yyscan;
    scanner->source = NULL;
    
    YY_BUFFER_STATE buffer = yy_scan_string(prompt, yyscan);
    if (buffer == NULL) {
        free(scanner);
        yylex_destroy(yyscan);
        return XERROR("clox_scanner", SCANNER_YY_CREATE_BUFFER_FAILED, "Failed to create scanner buffer");
    }
    scanner->buffer = buffer;

    return 0;
}

xerror* initScannerFile(Scanner** scanner_ptr, const char* path) {
    return initScannerImpl(scanner_ptr, path, createFileBuffer);
}

xerror* initScannerPrompt(Scanner** scanner_ptr, const char* prompt) {
    return initScannerImpl(scanner_ptr, prompt, createStringBuffer);
}

void freeScanner(Scanner* scanner) {
    if (scanner == NULL) {
        return;
    }
    if(scanner->source) {
        fclose(scanner->source);
    }
    yy_delete_buffer(scanner->buffer, scanner->yyscan);
    yylex_destroy(scanner->yyscan);
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
