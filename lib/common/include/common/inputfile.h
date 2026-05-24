#ifndef __CLOX_TEXTFILE_H__
#define __CLOX_TEXTFILE_H__

#include <stdbool.h>
#include <stdio.h>

#include <common/xerror.h>

typedef enum {
    INPUT_FILE_ERROR_FILE_OPEN = 1,
    INPUT_FILE_ERROR_ALLOC,
    INPUT_FILE_ERROR_FILE_READ,
    INPUT_FILE_ERROR_LAST = INPUT_FILE_ERROR_FILE_READ
} InputFileErrorCode;

typedef struct  {
    char* path;
    char* content;
    size_t size;
} InputFile;

xerror* readInputFile(const char* path, InputFile* out);

void freeInputFile(InputFile* file);

#endif // __CLOX_TEXTFILE_H__
