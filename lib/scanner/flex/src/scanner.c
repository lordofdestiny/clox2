#include <stdlib.h>

#include <scanner/gen/scanner_impl.h>

#include <scanner/scanner.h>
#include <scanner/token.h>

enum ScannerErrorCode : int {
    SCANNER_SUCCESS = 0,
    SCANNER_ERROR_SCANNER_DEST_NULL,
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

typedef  int(*CreateBufferType)(Scanner*, yyscan_t, const char*);

static int initScannerImpl(Scanner** scanner_ptr, const char* text, CreateBufferType createBuffer) {
    if (scanner_ptr == NULL) {
        return SCANNER_ERROR_SCANNER_DEST_NULL;
    }

    if (text == NULL) {
        return SCANNER_ERROR_FILE_NULL;
    }

    Scanner* scanner = malloc(sizeof(Scanner));
    if(scanner == NULL) {
        return SCANNER_ERROR_ALLOC_FAILED;
    }

    yyscan_t yyscan;
    if(yylex_init(&yyscan) != 0) {
        free(scanner);
        return SCANNER_YY_INIT_FAILED;
    }
    
    int code = createBuffer(scanner, yyscan, text);
    if (code != 0)  {
        scanner->yyscan = NULL;
        scanner->buffer = NULL;
        scanner->source = NULL;
        return code;
    }

    yy_switch_to_buffer(scanner->buffer, yyscan);
    yyset_lineno(1, yyscan);
    yyset_column(1, yyscan);

    *scanner_ptr = scanner;

    return SCANNER_SUCCESS;
}


static int createFileBuffer(Scanner* scanner, yyscan_t yyscan, const char* path) {
    scanner->yyscan = yyscan;

    FILE* file = fopen(path, "r");
    if (file == NULL) {
        free(scanner);
        yylex_destroy(yyscan);
        return SCANNER_ERROR_FILE_OPEN_FAILED;
    }
    scanner->source = file;
    
    YY_BUFFER_STATE buffer = yy_create_buffer(file,YY_BUF_SIZE, yyscan);
    if(buffer == NULL) {
        fclose(file);
        free(scanner);
        yylex_destroy(yyscan);
        return SCANNER_YY_CREATE_BUFFER_FAILED;
    }
    scanner->buffer = buffer;

    return 0;
}

static int createStringBuffer(Scanner* scanner, yyscan_t yyscan, const char* prompt) {
    scanner->yyscan = yyscan;
    scanner->source = NULL;
    
    YY_BUFFER_STATE buffer = yy_scan_string(prompt, yyscan);
    if (buffer == NULL) {
        free(scanner);
        yylex_destroy(yyscan);
        return SCANNER_YY_CREATE_BUFFER_FAILED;
    }
    scanner->buffer = buffer;

    return 0;
}

int initScannerFile(Scanner** scanner_ptr, const char* path) {
    return initScannerImpl(scanner_ptr, path, createFileBuffer);
}

int initScannerPrompt(Scanner** scanner_ptr, const char* prompt) {
    return initScannerImpl(scanner_ptr, prompt, createStringBuffer);
}

void freeScanner(Scanner* scanner) {
    if(scanner->source) {
        fclose(scanner->source);
    }
    if (scanner->buffer) {
        yy_delete_buffer(scanner->buffer, scanner->yyscan);
    }
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

static char* errorMessages[] = {
    [SCANNER_SUCCESS] = NULL,
    [SCANNER_ERROR_SCANNER_DEST_NULL] = "scanner destination is null",
    [SCANNER_ERROR_FILE_NULL] = "input file is null",
    [SCANNER_ERROR_FILE_OPEN_FAILED] = "failed to open input file",
    [SCANNER_ERROR_ALLOC_FAILED] = "buffer allocation failed",
    [SCANNER_YY_INIT_FAILED] = "failed to initialize flex scanner",
    [SCANNER_YY_CREATE_BUFFER_FAILED] = "failed to create flex buffer",
};

int formatScannerError(char* buffer, size_t cap, const char*, ScannerErrorCode cause) {
    int trueCause = cause & ((1 << 4) - 1);

    if (trueCause == SCANNER_SUCCESS) {
        return 0;
    }

    if (trueCause < SCANNER_SUCCESS || trueCause > SCANNER_ERROR_LAST) {
        return 0;
    }

    return snprintf(
        buffer, cap,
        "Failed to create scanner: %s\n\n",
        errorMessages[trueCause]);
}