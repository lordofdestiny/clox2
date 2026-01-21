#ifndef __CLOX2_SCANNER_PRIV_H__
#define __CLOX2_SCANNER_PRIV_H__

#include <stdbool.h>

#include "token.h"

#include "scanner.h"

extern Scanner* scanner;

bool isAtEnd();
char advance();
char peekNext();
char match(const char expected);

Token makeToken(const TokenType type);
Token errorToken(const char* message);

TokenType checkKeyword(
	int start, int length, const char* rest, TokenType type
);

#endif //__CLOX2_SCANNER_H__
