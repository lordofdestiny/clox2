//
// Created by djumi on 10/29/2023.
//

#ifndef CLOX2_SCANNER_H
#define CLOX2_SCANNER_H

typedef enum {
    // Single-character tokens
    TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
    TOKEN_LEFT_BRACKET, TOKEN_RIGHT_BRACKET,
    TOKEN_COMMA, TOKEN_COLON, TOKEN_DOT, TOKEN_VERTICAL_LINE,
    TOKEN_MINUS, TOKEN_PERCENT, TOKEN_PLUS, TOKEN_SEMICOLON,
    TOKEN_SLASH, TOKEN_STAR, TOKEN_QUESTION,

    // One or two character tokens
    TOKEN_BANG, TOKEN_BANG_EQUAL,
    TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
    TOKEN_GREATER, TOKEN_GREATER_EQUAL,
    TOKEN_LESS, TOKEN_LESS_EQUAL,
    TOKEN_PLUS_EQUAL, TOKEN_MINUS_EQUAL,
    TOKEN_STAR_EQUAL, TOKEN_SLASH_EQUAL,
    TOKEN_PERCENT_EQUAL,

    // Literals
    TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,

    // Keywords
    TOKEN_AND, TOKEN_AS, TOKEN_BREAK, TOKEN_CASE,
    TOKEN_CATCH, TOKEN_CONTINUE, TOKEN_CLASS, TOKEN_DEFAULT,
    TOKEN_ELSE, TOKEN_FALSE, TOKEN_FINALLY, TOKEN_FOR,
    TOKEN_FUN, TOKEN_IF, TOKEN_NIL, TOKEN_OR,
    TOKEN_PRINT, TOKEN_RETURN, TOKEN_SUPER, TOKEN_STATIC,
    TOKEN_SWITCH, TOKEN_THIS, TOKEN_THROW, TOKEN_TRUE,
    TOKEN_TRY, TOKEN_VAR, TOKEN_WHILE,

    TOKEN_ERROR, TOKEN_EOF
} TokenType;

typedef struct {
    TokenType type;
    const char* start;
    int length;
    int line;
} Token;

void initScanner(const char* source);

Token scanToken();

#endif //CLOX2_SCANNER_H
