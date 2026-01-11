#ifndef __CLOX2_TOKEN_H__
#define __CLOX2_TOKEN_H__

#include "scanner_generated_decls.h"

typedef struct {
    TokenType type;
    const char* start;
    int length;
    int line;
} Token;

#endif //__CLOX2_TOKEN_H__
