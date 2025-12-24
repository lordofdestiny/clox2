#include <string.h>

#include "scanner_priv.h"

static TokenLocation location() {
    return (TokenLocation) {
        .line = scanner->line,
        .column = scanner->column
    };
}

Token makeToken(const TokenType type) {
    return (Token) {
        .type = type,
        .start = scanner->start,
        .length = scanner->current - scanner->start,
        .loc = location()
    };
}

Token errorToken(const char* message) {
    return (Token) {
        .type = TOKEN_ERROR,
        .start = message,
        .length = strlen(message),
        .loc = location()
    };
}

bool isAtEnd() {
    return *scanner->current == '\0';
}

char advance() {
    scanner->current++;
    return scanner->current[-1];
}

char peekNext() {
    if (isAtEnd()) return '\0';
    return scanner->current[1];
}

char match(const char expected) {
    if (isAtEnd()) return false;
    if (*scanner->current != expected) return false;
    scanner->current++;
    return true;
}
