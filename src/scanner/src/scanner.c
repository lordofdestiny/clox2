#include <stdbool.h>
#include <string.h>
#include <stddef.h>
#include <ctype.h>

#include "scanner.h"
#include "scanner_priv.h"
#include "scanner_generated.h"

void initScanner(Scanner* scanner, InputFile source) {
    scanner->start = source.content;
    scanner->current = source.content;
    scanner->line = 1;
    scanner->column = 1;
}

void freeScanner(Scanner* scanner_arg) {
    (void) scanner_arg;
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

static char peek(Scanner* scanner) {
    return *scanner->current;
}

static void skipWhitespace(Scanner* scanner) {
    while (true) {
        const char c = peek(scanner);
        switch (c) {
        case '\n':
            scanner->line++;
            scanner->column = 0;
            [[fallthrough]];
        case ' ':
        case '\r':
        case '\t': {
            advance(scanner);
            scanner->column++;
            break;
        }
        case '/':
            if (peekNext(scanner) == '/') {
                while (peek(scanner) != '\n' && !isAtEnd(scanner)) advance(scanner);
                break;
            }
        default: return;
        }
    }
}

static Token identifier(Scanner* scanner) {
    while (isAlpha(peek(scanner)) || isDigit(peek(scanner))) advance(scanner);
    return makeToken(scanner, identifierType(scanner));
}

static Token number(Scanner* scanner) {
    while (isDigit(peek(scanner))) advance(scanner);

    if (peek(scanner) == '.' && isDigit(peekNext(scanner))) {
        advance(scanner);

        while (isDigit(peek(scanner))) advance(scanner);
    }

    return makeToken(scanner, TOKEN_NUMBER);
}

bool isEscapable(char c) {
    static const char* chars = "abfrntv\\\'\"";
    return strchr(chars, c);
}

#define STRING_ERROR(str) do{errorMsg = str; goto skip; } while(0);

static Token string(Scanner* scanner) {
    const char* errorMsg = NULL;
    while (peek(scanner) != '"' && !isAtEnd(scanner)) {
        if (peek(scanner) == '\n') {
            scanner->line++;
            scanner->column = 1;
            STRING_ERROR( "unterminated string literal");
        }
        if (peek(scanner) != '\\') {
            advance(scanner);
            continue;
        }

        // Escape sequences
        // consume backslash
        advance(scanner);
        if (isAtEnd(scanner)) {
            STRING_ERROR( "unterminated string literal");
        }
        if (isEscapable(peek(scanner))) {
            advance(scanner);
            continue;
        }
        if (peek(scanner) == 'x') {
            advance(scanner);
            int i = 0;
            long total = 0;
            bool outOfRange = false;
            while(isHexDigit(peek(scanner)) && !isAtEnd(scanner)) {
                char c = advance(scanner);
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
        if (isOctDigit(peek(scanner))) {
            int i = 0;
            int total = 0;
            bool outOfRange = false;
            while(i < 3 && isOctDigit(peek(scanner)) && !isAtEnd(scanner)) {
                char c = advance(scanner);
                total = 8 * total + (c - '0');
                if (total > 0xff) outOfRange = true;
                i++;
            }
            if (outOfRange) STRING_ERROR("octal escape sequence out of range");
            continue;
        }
        STRING_ERROR("unknown escape sequence");
    }

    if (isAtEnd(scanner)) {
        return errorToken(scanner, "unterminated string literal");
    }

skip:
    while (peek(scanner) != '"' && !isAtEnd(scanner)) advance(scanner);
    // The closing quote
    advance(scanner);    
    if (errorMsg != NULL) {
        return errorToken(scanner, errorMsg);
    }
    return makeToken(scanner, TOKEN_STRING);
}

TokenType checkKeyword(
	Scanner* scanner, int start, int length, const char* rest, TokenType type
) {
	if (scanner->current - scanner->start == (ptrdiff_t) start + length &&
		memcmp(scanner->start + start, rest, length) == 0) {
		return type;
	}
	return TOKEN_IDENTIFIER;
}

Token scanToken(Scanner* scanner) {
    scanner->column += (scanner->current - scanner->start);
    skipWhitespace(scanner);
    scanner->start = scanner->current;

    if (isAtEnd(scanner)) return makeToken(scanner, TOKEN_EOF);

    const char c = advance(scanner);
    if (isAlpha(c)) return identifier(scanner);
    if (isDigit(c)) return number(scanner);
    if (c == '"') return string(scanner);

    return charToken(scanner, c);
}
