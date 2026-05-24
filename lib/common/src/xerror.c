#include <common/xerror.h>
#include <common/arena.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct xerror {
    struct xerror* cause;
    const char* lib_name;
    const char* context;
    const char* file;
    const char* function;
    int line;
    int code;
};

static void xperror_single(xerror *err) {
    if (err->lib_name) {
        const char * name = strcmp(err->lib_name, XERROR_LIBC_NAME) == 0 ? "libc" : err->lib_name;
        fprintf(stderr, "[%s] ", name);
    }

    fprintf(stderr, "(%s:%d in \"%s\")\n\t", err->file, err->line, err->function);
    if (err->context) {
        fprintf(stderr, "%s", err->context);
    }

    if (err->lib_name && strcmp(err->lib_name, XERROR_LIBC_NAME) == 0) {
        fprintf(stderr, ": %s\n", strerror(err->code));
    } else {
        fprintf(stderr, " (Code: %d)\n", err->code);
    }
}

void xperror(xerror *err) {
    if (err == NULL) return;

    fprintf(stderr, "Error:\n");
    while (err != NULL) {
        xperror_single(err);
        err = err->cause;
    }
}

xerror* xerror_create(
    xerror* cause, const char* lib_name, int code, const char* context,
    const char* file, int line, const char* function
) {
    xerror* error = malloc(sizeof(xerror));
    if (error == NULL) {
        return NULL;
    }
    
    error->cause = cause;
    error->lib_name = lib_name;
    error->context = context;
    error->file = file;
    error->function = function;
    error->line = line;
    error->code = code;
    
    return error;
}

static void xerror_free_single(xerror *err) {
    free(err);
}

void xerror_free(xerror *err) {
    while (err != NULL) {
        xerror *next = err->cause;
        xerror_free_single(err);
        err = next;
    }
}
