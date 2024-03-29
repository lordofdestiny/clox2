//
// Created by djumi on 10/29/2023.
//

#include <stdio.h>
#include <string.h>

#include "../h/common.h"
#include "../h/scanner.h"

typedef struct {
    const char* start;
    const char* current;
    int line;
} Scanner;

Scanner scanner;

void initScanner(const char* source) {
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

bool isAlpha(const char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           c == '_';
}

bool isDigit(const char c) {
    return c >= '0' && c <= '9';
}

static bool isAtEnd() {
    return *scanner.current == '\0';
}


static char advance() {
    scanner.current++;
    return scanner.current[-1];
}

static char peek() {
    return *scanner.current;
}

static char peekNext() {
    if (isAtEnd()) return '\0';
    return scanner.current[1];
}

static char match(const char expected) {
    if (isAtEnd()) return false;
    if (*scanner.current != expected) return false;
    scanner.current++;
    return true;
}

static Token makeToken(const TokenType type) {
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int) (scanner.current - scanner.start);
    token.line = scanner.line;
    return token;
}

static Token errorToken(const char* message) {
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int) strlen(message);
    token.line = scanner.line;
    return token;
}

static void skipWhitespace() {
    while (true) {
        const char c = peek();
        switch (c) {
        case ' ':
        case '\r':
        case '\t': {
            advance();
            break;
        }
        case '\n': {
            scanner.line++;
            advance();
            break;
        }
        case '/':
            if (peekNext() == '/') {
                while (peek() != '\n' && !isAtEnd()) advance();
            } else {
                return;
            }
            break;
        default: return;
        }
    }
}

static TokenType checkKeyword(
    const int start, const int length, const char* rest, const TokenType type
) {
    if (scanner.current - scanner.start == (ptrdiff_t) start + length &&
        memcmp(scanner.start + start, rest, length) == 0) {
        return type;
    }
    return TOKEN_IDENTIFIER;
}

static TokenType identifierType() {
    switch (scanner.start[0]) {
    case 'a':
        if (scanner.current - scanner.start > 1) {
            switch (scanner.start[1]) {
            case 'n': return checkKeyword(2, 1, "d", TOKEN_AND);
            case 's': return checkKeyword(2, 0, "", TOKEN_AS);
            default: ;
            }
        }
    case 'b': return checkKeyword(1, 4, "reak", TOKEN_BREAK);
    case 'c':
        if (scanner.current - scanner.start > 1) {
            switch (scanner.start[1]) {
            case 'a':
                if (scanner.current - scanner.start > 2) {
                    switch (scanner.start[2]) {
                    case 's': return checkKeyword(3, 1, "e", TOKEN_CASE);
                    case 't': return checkKeyword(3, 2, "ch", TOKEN_CATCH);
                    default: ;
                    }
                }
            case 'l': return checkKeyword(2, 3, "ass", TOKEN_CLASS);
            case 'o': return checkKeyword(2, 6, "ntinue", TOKEN_CONTINUE);
            default: ;
            }
        }
    case 'd': return checkKeyword(1, 6, "efault", TOKEN_DEFAULT);
    case 'e': return checkKeyword(1, 3, "lse", TOKEN_ELSE);
    case 'f':
        if (scanner.current - scanner.start > 1) {
            switch (scanner.start[1]) {
            case 'a': return checkKeyword(2, 3, "lse", TOKEN_FALSE);
            case 'o': return checkKeyword(2, 1, "r", TOKEN_FOR);
            case 'u': return checkKeyword(2, 1, "n", TOKEN_FUN);
            case 'i': return checkKeyword(2, 5, "nally", TOKEN_FINALLY);
            default: ;
            }
        }
        break;
    case 'i': return checkKeyword(1, 1, "f", TOKEN_IF);
    case 'n': return checkKeyword(1, 2, "il", TOKEN_NIL);
    case 'o': return checkKeyword(1, 1, "r", TOKEN_OR);
    case 'p': return checkKeyword(1, 4, "rint", TOKEN_PRINT);
    case 'r': return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
    case 's':
        if (scanner.current - scanner.start > 1) {
            switch (scanner.start[1]) {
            case 't': return checkKeyword(2, 4, "atic", TOKEN_STATIC);
            case 'u': return checkKeyword(2, 3, "per", TOKEN_SUPER);
            case 'w': return checkKeyword(2, 4, "itch", TOKEN_SWITCH);
            default: ;
            }
        }
    case 't':
        if (scanner.current - scanner.start > 1) {
            switch (scanner.start[1]) {
            case 'h':
                if (scanner.current - scanner.start > 2) {
                    switch (scanner.start[2]) {
                    case 'i': return checkKeyword(3, 1, "s", TOKEN_THIS);
                    case 'r': return checkKeyword(3, 2, "ow", TOKEN_THROW);
                    default: ;
                    }
                }
            case 'r':
                if (scanner.current - scanner.start > 2) {
                    switch (scanner.start[2]) {
                    case 'u': return checkKeyword(3, 1, "e", TOKEN_TRUE);
                    case 'y': return checkKeyword(3, 0, "", TOKEN_TRY);
                    default: ;
                    }
                }
            default: ;
            }
        }
    case 'v': return checkKeyword(1, 2, "ar", TOKEN_VAR);
    case 'w': return checkKeyword(1, 4, "hile", TOKEN_WHILE);
    default: ;
    }
    return TOKEN_IDENTIFIER;
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

Token scanToken() {
    skipWhitespace();
    scanner.start = scanner.current;

    if (isAtEnd()) return makeToken(TOKEN_EOF);

    const char c = advance();
    if (isAlpha(c)) return identifier();
    if (isDigit(c)) return number();

    switch (c) {
    case '(': return makeToken(TOKEN_LEFT_PAREN);
    case ')': return makeToken(TOKEN_RIGHT_PAREN);
    case '{': return makeToken(TOKEN_LEFT_BRACE);
    case '}': return makeToken(TOKEN_RIGHT_BRACE);
    case '[': return makeToken(TOKEN_LEFT_BRACKET);
    case ']': return makeToken(TOKEN_RIGHT_BRACKET);
    case ';': return makeToken(TOKEN_SEMICOLON);
    case ',': return makeToken(TOKEN_COMMA);
    case ':': return makeToken(TOKEN_COLON);
    case '.': return makeToken(TOKEN_DOT);
    case '?': return makeToken(TOKEN_QUESTION);
    case '|': return makeToken(TOKEN_VERTICAL_LINE);
    case '-': return makeToken(match('=') ? TOKEN_MINUS_EQUAL : TOKEN_MINUS);
    case '+': return makeToken(match('=') ? TOKEN_PLUS_EQUAL : TOKEN_PLUS);
    case '/': return makeToken(match('=') ? TOKEN_SLASH_EQUAL : TOKEN_SLASH);
    case '*': {
        if (match('*')) return makeToken(TOKEN_STAR_STAR);
        if (match('=')) return makeToken(TOKEN_STAR_EQUAL);
        return makeToken(TOKEN_STAR);
    }
    case '%': return makeToken(match('=') ? TOKEN_PERCENT_EQUAL : TOKEN_PERCENT);
    case '!': return makeToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
    case '=': return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
    case '>': return makeToken(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
    case '<': return makeToken(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
    case '"': return string();
    default: break;
    }

    return errorToken("Unexpected character.");
}
