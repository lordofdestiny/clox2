#ifndef __CLOX2_SCANNER_H__
#define __CLOX2_SCANNER_H__

#include <common/inputfile.h>

#include <scanner/token.h>

typedef struct {
    const char* start;
    const char* current;
    int line;
    int column;
} Scanner;

void initScanner(Scanner* scanner, InputFile source);
void freeScanner(Scanner* scanner);
Token scanToken(Scanner* scanners);

#endif //__CLOX2_SCANNER_H__
