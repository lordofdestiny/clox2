#ifndef __CLOX2_SCANNER_H__
#define __CLOX2_SCANNER_H__

#include "token.h"
#include "inputfile.h"

typedef struct {
    const char* start;
    const char* current;
    int line;
    int column;
} Scanner;

__attribute__((visibility("default"))) void initScanner(Scanner* scanner, InputFile source);
__attribute__((visibility("default"))) void freeScanner(Scanner* scanner);
__attribute__((visibility("default"))) Token scanToken(Scanner* scanners);

#endif //__CLOX2_SCANNER_H__
