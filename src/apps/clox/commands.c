#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "commands.h"

static char* readFile(const char* path, size_t* out_size) {
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

    *out_size = fileSize;
    return buffer;
}

TextFile readTextFile(const char* path) {
    TextFile file = {
        .path = NULL,
        .content = NULL,
        .size = 0
    };

    file.path = strdup(path);
    if (file.path == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(ERROR_FAILED_TO_READ_FILE);
    }

    file.content = readFile(path, &file.size);

    return file;
}


void freeTextFile(TextFile* file) {
    free(file->path);
    free(file->content);

    memset(file, 0, sizeof(TextFile));
}