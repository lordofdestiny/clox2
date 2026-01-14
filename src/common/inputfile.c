#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "inputfile.h"

static InputFile makeFile(char* path, char* src, size_t size) {
    return (InputFile) {
        .path = path,
        .content = src,
        .size = size  
    };
}

InputFileErrorCode readInputFile(const char* path, InputFile* out) {
    char* pathCopy = strdup(path);
    if (pathCopy == NULL) {
        return INPUT_FILE_ERROR_ALLOC_FAILED;
    }

    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        return INPUT_FILE_ERROR_FILE_OPEN_FAILED;
    }

    fseek(file, 0L, SEEK_END);
    const size_t size = ftell(file);
    rewind(file);

    char* buffer = (char*) calloc(size + 2, 1);
    if (buffer == NULL) {
        return INPUT_FILE_ERROR_ALLOC_FAILED;
    }

    const size_t bytesRead = fread(buffer, sizeof(char), size, file);
    if (bytesRead < size) {
        return INPUT_FILE_ERROR_FILE_READ_FAILED;
    }

    fclose(file);

    *out = makeFile(pathCopy, buffer, size);
    return INPUT_FILE_SUCCESS;
}

void freeInputFile(InputFile* file) {
    free(file->path);
    free(file->content);
    memset(file, 0, sizeof(InputFile));
}

// fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
// fprintf(stderr, "Could not open file \"%s\".\n", path);
// fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
// fprintf(stderr, "Could not read file \"%s\".\n", path);

static char* errorMessages[] = {
    [INPUT_FILE_SUCCESS] = NULL,
    [INPUT_FILE_ERROR_ALLOC_FAILED] = "buffer allocation failed",
    [INPUT_FILE_ERROR_FILE_OPEN_FAILED] = "could to open the file",
    [INPUT_FILE_ERROR_FILE_READ_FAILED] = "could to read the file",
};

int formatInputFileError(char* buffer, size_t cap, const char* file, InputFileErrorCode cause) {
    if (cause == INPUT_FILE_SUCCESS || file == NULL) {
        return 0;
    }

    if (cause < INPUT_FILE_SUCCESS || cause > INPUT_FILE_ERROR_LAST) {
        return 0;
    }

    return snprintf(
        buffer, cap,
        "Failed to read the input file \"%s\": %s",
        file, errorMessages[cause]);
}
