#ifndef __CLOX2_COMMANDS_H__
#define __CLOX2_COMMANDS_H__

#include <stdbool.h>
#include <stdio.h>

#define ERROR_FAILED_TO_READ_FILE 74

typedef struct {
    char* start;
    char* end;
} TextFileLine;

typedef struct {
    TextFileLine* lines;
    unsigned count;
} TextFileMap;

typedef struct  {
    char* path;
    char* content;
    TextFileMap lineMap;
    size_t size;
} TextFile;

TextFile readTextFile(const char* path, bool linesMap);
void freeTextFile(TextFile* file);

#endif //__CLOX2_COMMANDS_H__