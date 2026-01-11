#ifndef __CLOX2_COMMANDS_H__
#define __CLOX2_COMMANDS_H__

#include <stdbool.h>
#include <stdio.h>

#define ERROR_FAILED_TO_READ_FILE 74

typedef struct  {
    char* path;
    char* content;
    size_t size;
} TextFile;

TextFile readTextFile(const char* path);
void freeTextFile(TextFile* file);

#endif //__CLOX2_COMMANDS_H__