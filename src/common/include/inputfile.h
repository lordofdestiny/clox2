#ifndef __CLOX_TEXTFILE_H__
#define __CLOX_TEXTFILE_H__

#include <stdbool.h>
#include <stdio.h>

typedef enum {
    INPUT_FILE_SUCCESS = 0,
    INPUT_FILE_ERROR_FILE_OPEN_FAILED,
    INPUT_FILE_ERROR_ALLOC_FAILED,
    INPUT_FILE_ERROR_FILE_READ_FAILED,
    INPUT_FILE_ERROR_LAST = INPUT_FILE_ERROR_FILE_READ_FAILED
} InputFileErrorCode;

typedef struct  {
    char* path;
    char* content;
    size_t size;
} InputFile;

InputFileErrorCode readInputFile(const char* path, InputFile* out);

void freeInputFile(InputFile* file);

int formatInputFileError(char* buffer, size_t cap, const char* file, InputFileErrorCode cause);

#endif // __CLOX_TEXTFILE_H__
