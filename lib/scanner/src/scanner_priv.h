#ifndef __CLOX2_SCANNER_PRIV_H__
#define __CLOX2_SCANNER_PRIV_H__

#include <stdbool.h>

#include <scanner/scanner.h>
#include <scanner/token.h>

bool isAtEnd(Scanner* scanner);
char advance(Scanner* scanner);
char peekNext(Scanner* scanner);
char match(Scanner* scanner, const char expected);

Token makeToken(Scanner* scanner, const TokenType type);
Token errorToken(Scanner* scanner, const char* message);

TokenType checkKeyword(
	Scanner* scanner, int start, int length, const char* rest, TokenType type
); 

#endif //__CLOX2_SCANNER_H__
