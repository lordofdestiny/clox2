#include <string.h>

#include "scanner_priv.h"

Token makeToken(const TokenType type) {
    return (Token) {
        .type = type,
        .start = scanner.start,
        .length = scanner.current - scanner.start,
        .line = scanner.line,
    };
}

Token errorToken(const char* message) {
    return (Token) {
        .type = TOKEN_ERROR,
        .start = message,
        .length = strlen(message),
        .line = scanner.line
    };
}

bool isAtEnd() {
    return *scanner.current == '\0';
}

char advance() {
    scanner.current++;
    return scanner.current[-1];
}

char peekNext() {
    if (isAtEnd()) return '\0';
    return scanner.current[1];
}

char match(const char expected) {
    if (isAtEnd()) return false;
    if (*scanner.current != expected) return false;
    scanner.current++;
    return true;
}