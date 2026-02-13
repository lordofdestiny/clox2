#ifndef __CLOX2_TOKEN_H__
#define __CLOX2_TOKEN_H__

#include <scanner/gen/decls.h>

typedef struct {
	int line;
	int column;
} TokenLocation;

typedef struct {
    TokenType type;
    TokenLocation loc;
    const char* start;
    int length;
} Token;

#endif //__CLOX2_TOKEN_H__
