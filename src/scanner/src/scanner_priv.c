#include <string.h>

#include "scanner_priv.h"

static TokenLocation location(Scanner* scanner) {
    return (TokenLocation) {
        .line = scanner->line,
        .column = scanner->column
    };
}

Token makeToken(Scanner* scanner, const TokenType type) {
    return (Token) {
        .type = type,
        .start = scanner->start,
        .length = scanner->current - scanner->start,
        .loc = location(scanner)
    };
}

Token errorToken(Scanner* scanner, const char* message) {
    return (Token) {
        .type = TOKEN_ERROR,
        .start = message,
        .length = strlen(message),
        .loc = location(scanner)
    };
}

bool isAtEnd(Scanner* scanner) {
    return *scanner->current == '\0';
}

char advance(Scanner* scanner) {
    scanner->current++;
    return scanner->current[-1];
}

char peekNext(Scanner* scanner) {
    if (isAtEnd(scanner)) return '\0';
    return scanner->current[1];
}

char match(Scanner* scanner, const char expected) {
    if (isAtEnd(scanner)) return false;
    if (*scanner->current != expected) return false;
    scanner->current++;
    return true;
}
