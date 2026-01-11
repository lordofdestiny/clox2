#include <stdbool.h>
#include <string.h>
#include <stddef.h>

#include "scanner.h"
#include "scanner_priv.h"
#include "scanner_generated.h"

Scanner scanner;

void initScanner(const char* source) {
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

static bool isAlpha(const char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           c == '_';
}

static bool isDigit(const char c) {
    return c >= '0' && c <= '9';
}

static char peek() {
    return *scanner.current;
}

static void skipWhitespace() {
    while (true) {
        const char c = peek();
        switch (c) {
        case '\n':
            scanner.line++;
        case ' ':
        case '\r':
        case '\t': {
            advance();
            break;
        }
        case '/':
            if (peekNext() == '/') {
                while (peek() != '\n' && !isAtEnd()) advance();
                break;
            }
        default: return;
        }
    }
}

static Token identifier() {
    while (isAlpha(peek()) || isDigit(peek())) advance();
    return makeToken(identifierType());
}

static Token number() {
    while (isDigit(peek())) advance();

    if (peek() == '.' && isDigit(peekNext())) {
        advance();

        while (isDigit(peek())) advance();
    }

    return makeToken(TOKEN_NUMBER);
}

static Token string() {
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') scanner.line++;
        advance();
    }

    if (isAtEnd()) return errorToken("Unterminated string.");

    // The closing quote
    advance();
    return makeToken(TOKEN_STRING);
}

TokenType checkKeyword(
	int start, int length, const char* rest, TokenType type
) {
	if (scanner.current - scanner.start == (ptrdiff_t) start + length &&
		memcmp(scanner.start + start, rest, length) == 0) {
		return type;
	}
	return TOKEN_IDENTIFIER;
}

Token scanToken() {
    skipWhitespace();
    scanner.start = scanner.current;

    if (isAtEnd()) return makeToken(TOKEN_EOF);

    const char c = advance();
    if (isAlpha(c)) return identifier();
    if (isDigit(c)) return number();
    if (c == '"') return string();

    return charToken(c);
}