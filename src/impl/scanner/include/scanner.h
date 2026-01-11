#ifndef __CLOX2_SCANNER_H__
#define __CLOX2_SCANNER_H__

#include "visibility.h"
#include "token.h"

PRIVATE void initScanner(const char* source);
PRIVATE Token scanToken();

#endif //__CLOX2_SCANNER_H__
