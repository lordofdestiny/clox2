#ifndef __CLOX2_SCANNER_H__
#define __CLOX2_SCANNER_H__

#include "visibility.h"
#include "token.h"
#include "inputfile.h"

typedef struct {
    const char* start;
    const char* current;
    int line;
    int column;
} Scanner;

PUBLIC void initScanner(Scanner* scanner, InputFile source);
PUBLIC void freeScanner(Scanner* scanner);
PUBLIC Token scanToken(Scanner* scanners);

#endif //__CLOX2_SCANNER_H__
