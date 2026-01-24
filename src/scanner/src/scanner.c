#include <stdbool.h>
#include <string.h>
#include <stddef.h>
#include <ctype.h>

#include "scanner.h"
#include "scanner_priv.h"
#include "scanner_generated.h"

Scanner* scanner;

void initScanner(Scanner* scanner_arg, InputFile source) {
    scanner = scanner_arg;
    scanner->start = source.content;
    scanner->current = source.content;
    scanner->line = 1;
    scanner->column = 1;
}

void freeScanner([[maybe_unused]] Scanner* scanner_arg) {
    scanner = NULL;
}

static bool isAlpha(const char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           c == '_';
}

static bool isDigit(const char c) {
    return isdigit(c);
}

static bool isOctDigit(const char c) {
    return isDigit(c) && c <= '7';
}

static bool isHexDigit(const char c) {
    return isxdigit(c);
}

static char peek() {
    return *scanner->current;
}

static void skipWhitespace() {
    while (true) {
        const char c = peek();
        switch (c) {
        case '\n':
            scanner->line++;
            scanner->column = 0;
            [[fallthrough]];
        case ' ':
        case '\r':
        case '\t': {
            advance();
            scanner->column++;
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

bool isEscapable(char c) {
    static const char* chars = "abfrntv\\\'\"";
    return strchr(chars, c);
}

#define STRING_ERROR(str) do{errorMsg = str; goto skip; } while(0);

static Token string() {
    const char* errorMsg = NULL;
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') {
            scanner->line++;
            scanner->column = 1;
            STRING_ERROR( "unterminated string literal");
        }
        if (peek() != '\\') {
            advance();
            continue;
        }

        // Escape sequences
        // consume backslash
        advance();
        if (isAtEnd()) {
            STRING_ERROR( "unterminated string literal");
        }
        if (isEscapable(peek())) {
            advance();
            continue;
        }
        if (peek() == 'x') {
            advance();
            int i = 0;
            long total = 0;
            bool outOfRange = false;
            while(isHexDigit(peek()) && !isAtEnd()) {
                char c = advance();
                total *= 16;
                if (isDigit(c)) total += c - '0';
                if (islower(c)) total += c - 'a';
                if (isupper(c)) total += c - 'A';
                if (total > 0xff) outOfRange = true;
                i++;
            }
            if (i == 0) STRING_ERROR("\\x used with no following hex digits");
            if (outOfRange) STRING_ERROR("hex escape sequence out of range");
            continue;
        }
        if (isOctDigit(peek())) {
            int i = 0;
            int total = 0;
            bool outOfRange = false;
            while(i < 3 && isOctDigit(peek()) && !isAtEnd()) {
                char c = advance();
                total = 8 * total + (c - '0');
                if (total > 0xff) outOfRange = true;
                i++;
            }
            if (outOfRange) STRING_ERROR("octal escape sequence out of range");
            continue;
        }
        STRING_ERROR("unknown escape sequence");
    }

    if (isAtEnd()) return errorToken("unterminated string literal");

skip:
    while (peek() != '"' && !isAtEnd()) advance();
    // The closing quote
    advance();    
    if (errorMsg != NULL) {
        return errorToken(errorMsg);
    }
    return makeToken(TOKEN_STRING);
}

TokenType checkKeyword(
	int start, int length, const char* rest, TokenType type
) {
	if (scanner->current - scanner->start == (ptrdiff_t) start + length &&
		memcmp(scanner->start + start, rest, length) == 0) {
		return type;
	}
	return TOKEN_IDENTIFIER;
}

Token scanToken([[maybe_unused]] Scanner* scanner) {
    scanner->column += (scanner->current - scanner->start);
    skipWhitespace();
    scanner->start = scanner->current;

    if (isAtEnd()) return makeToken(TOKEN_EOF);

    const char c = advance();
    if (isAlpha(c)) return identifier();
    if (isDigit(c)) return number();
    if (c == '"') return string();

    return charToken(c);
}
