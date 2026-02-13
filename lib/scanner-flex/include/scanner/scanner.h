#ifndef __CLOX2_SCANNER_H__
#define __CLOX2_SCANNER_H__

#include <stddef.h>

#include <common/inputfile.h>

#include <scanner/token.h>

typedef struct {
    void* impl;
} Scanner;

void initScanner(Scanner* scanner, InputFile source);
void freeScanner(Scanner* scanner);
Token scanToken(Scanner* scanner);

#endif // __CLOX2_SCANNER_H__
