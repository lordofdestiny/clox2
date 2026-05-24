#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <common/inputfile.h>

static InputFile makeFile(char* path, char* src, size_t size) {
    return (InputFile) {
        .path = path,
        .content = src,
        .size = size  
    };
}

xerror* readInputFile(const char* path, InputFile* out) {
    char* pathCopy = strdup(path);
    if (pathCopy == NULL) {
        return XERROR_LIBC("Failed to duplicate file path");
    }

    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        free(pathCopy);
        return XERROR_LIBC("Failed to open file");
    }

    fseek(file, 0L, SEEK_END);
    long size = ftell(file);
    if (size < 0) {
        free(pathCopy);
        fclose(file);
        return XERROR_LIBC("Failed to determine file size");
    }
    rewind(file);

    char* buffer = malloc(size + 1);
    if (buffer == NULL) {
        free(pathCopy);
        fclose(file);
        return XERROR_LIBC("Failed to allocate buffer");
    }

    const size_t bytesRead = fread(buffer, sizeof(char), size, file);
    if (bytesRead < (size_t) size) {
        free(pathCopy);
        fclose(file);
        free(buffer);
        return XERROR_LIBC("Failed to read file");
    }
    buffer[size] = '\0';

    fclose(file);

    *out = makeFile(pathCopy, buffer, size);
    return NULL;
}

void freeInputFile(InputFile* file) {
    free(file->path);
    free(file->content);
    memset(file, 0, sizeof(InputFile));
}
