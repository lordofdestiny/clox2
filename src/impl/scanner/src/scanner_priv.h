#ifndef __CLOX2_SCANNER_PRIV_H__
#define __CLOX2_SCANNER_PRIV_H__

#include <stdbool.h>

#include "token.h"
#include "visibility.h"

typedef struct {
    const char* start;
    const char* current;
    int line;
} Scanner;

PRIVATE extern Scanner scanner;

PRIVATE bool isAtEnd();
PRIVATE char advance();
PRIVATE char peekNext();
PRIVATE char match(const char expected);

PRIVATE Token makeToken(const TokenType type);
PRIVATE Token errorToken(const char* message);

PRIVATE TokenType checkKeyword(
	int start, int length, const char* rest, TokenType type
);

#endif //__CLOX2_SCANNER_H__
