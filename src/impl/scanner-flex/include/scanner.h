#ifndef __CLOX2_SCANNER_H__
#define __CLOX2_SCANNER_H__

#include <stddef.h>

#include "inputfile.h"
#include "token.h"
#include "visibility.h"

typedef struct {
    void* impl;
} Scanner;

PUBLIC void initScanner(Scanner* scanner, InputFile source);
PUBLIC void freeScanner(Scanner* scanner);
PUBLIC Token scanToken(Scanner* scanner);

#endif // __CLOX2_SCANNER_H__
