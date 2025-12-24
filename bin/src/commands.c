#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "clox/commands.h"

static void buildLinesMap(TextFile* file) {
    return;
    if (file->content == NULL) {
        return;
    }

    size_t lineCount = 1;
    TextFileLine* newLines = (TextFileLine*) calloc(lineCount, sizeof(TextFileLine));
    size_t stackSize = 1;
    size_t stackCapacity = 1;
    char** stack = (char**) calloc(1, sizeof(char*));

    if (newLines == NULL || stack == NULL) {
        fprintf(stderr, "Not enough memory to build lines map.\n");
        exit(ERROR_FAILED_TO_READ_FILE);
    }

    stack[0] = file->content;
    while(stackSize > 0) {
        // TODO
    }
}

typedef struct {
    char* content;
    size_t size;
} ReadFile;

static ReadFile readFile(const char* path, bool linesMap) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(ERROR_FAILED_TO_READ_FILE);
    }

    fseek(file, 0L, SEEK_END);
    const size_t fileSize = ftell(file);
    rewind(file);

    char* buffer = (char*) malloc(fileSize + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(ERROR_FAILED_TO_READ_FILE);
    }

    const size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(ERROR_FAILED_TO_READ_FILE);
    }
    buffer[bytesRead] = '\0';

    fclose(file);

    return (ReadFile){.content = buffer, .size = bytesRead};
}

TextFile readTextFile(const char* path, bool linesMap) {
    TextFile file = {
        .path = NULL,
        .content = NULL,
        .lineMap = {NULL, 0}
    };

    file.path = strdup(path);
    if (file.path == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(ERROR_FAILED_TO_READ_FILE);
    }

    ReadFile rfile = readFile(path, linesMap);
    file.content = rfile.content;
    file.size = rfile.size;

    if (false || linesMap) {
        buildLinesMap(&file);
    }

    return file;
}


void freeTextFile(TextFile* file) {
    free(file->path);
    free(file->content);
    free(file->lineMap.lines);

    memset(file, 0, sizeof(TextFile));
}