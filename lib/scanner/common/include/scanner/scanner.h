#ifndef __CLOX2_SCANNER_H__
#define __CLOX2_SCANNER_H__

#include <common/inputfile.h>

#include <common/xerror.h>

#include <scanner/token.h>

enum ScannerErrorCode : int;
typedef enum ScannerErrorCode ScannerErrorCode;

typedef struct Scanner Scanner;

[[nodiscard("Scanner int might fail")]]
xerror* initScannerFile(Scanner** scanner, FILE* file);

[[nodiscard("Scanner int might fail")]]
xerror* initScannerPrompt(Scanner** scanner, const char* prompt);

void freeScanner(Scanner* scanner);

Token scanToken(Scanner* scanner);

int formatScannerError(char* buffer, size_t cap, const char* file, ScannerErrorCode cause);

#endif // __CLOX2_SCANNER_H__
