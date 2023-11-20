//
// Created by djumi on 11/19/2023.
//

#include <stdio.h>
#include <stdlib.h>

#include "../h/output.h"
#include "../h/memory.h"
#include "../h/common.h"

#define SAVE_FAILURE 44

typedef struct {
    Value *values;
    int count;
    int capacity;
    int head;
    int tail;
} ValueQueue;

static void initValueQueue(ValueQueue *queue) {
    queue->values = 0;
    queue->count = 0;
    queue->capacity = 0;
    queue->head = 0;
    queue->tail = 0;
}

static void freeValueQueue(ValueQueue *queue) {
    free(queue->values);
    initValueQueue(queue);
}

static bool findInQueue(ValueQueue *queue, Value function) {
    for (int j = queue->head; j < queue->tail; j++) {
        if (!IS_FUNCTION(queue->values[j])) continue;
        if (AS_FUNCTION(function) == AS_FUNCTION(queue->values[j]))
            return true;
    }
    return false;
}

static void enqueueValue(ValueQueue *queue, Value value) {
    if (queue->capacity < queue->count + 1) {
        queue->capacity = GROW_CAPACITY(queue->capacity);
        void *memory = realloc(queue->values, sizeof(Value) * queue->capacity);
        if (memory != NULL) {
            queue->values = (Value *) memory;
        } else {
            fprintf(stderr, "Memory error.\n");
            exit(SAVE_FAILURE);
        }
    }
    queue->values[queue->tail] = value;
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->count++;
}

static Value pollValue(ValueQueue *queue) {
    Value value = queue->values[queue->head];
    queue->head = (queue->head + 1) % queue->capacity;
    queue->count--;
    return value;
}

typedef struct {
    ObjFunction *function;
    int id;
} FunctionId;

typedef struct {
    int count;
    int capacity;
    FunctionId *ids;
} FunctionIdList;

static void initFunctionIdList(FunctionIdList *list) {
    list->count = 0;
    list->capacity = 0;
    list->ids = NULL;
}

static void freeFunctionIdList(FunctionIdList *list) {
    free(list->ids);
    initFunctionIdList(list);
}

static void addFunctionId(FunctionIdList *list, FunctionId id) {
    if (list->capacity < list->count + 1) {
        list->capacity = GROW_CAPACITY(list->capacity);
        void *memory = realloc(list->ids, sizeof(FunctionId) * list->capacity);
        if (memory != NULL) {
            list->ids = (FunctionId *) memory;
        } else {
            fprintf(stderr, "Memory error.\n");
            exit(SAVE_FAILURE);
        }
    }
    list->ids[list->count] = id;
    list->count++;
}

static FunctionId *findFunctionId(FunctionIdList *list, ObjFunction *function) {
    for (int i = 0; i < list->count; i++) {
        FunctionId *id = &list->ids[i];
        if (function == id->function) {
            return id;
        }
    }
    return NULL;
}

typedef struct {
    long position;
    ObjFunction *function;
} FilePatch;

typedef struct {
    int count;
    int capacity;
    FilePatch *patches;
} PatchList;

static void initPatchList(PatchList *list) {
    list->count = 0;
    list->capacity = 0;
    list->patches = NULL;
}

static void freePatchList(PatchList *list) {
    free(list->patches);
    initPatchList(list);
}

static void addPatch(PatchList *list, FilePatch id) {
    if (list->capacity < list->count + 1) {
        list->capacity = GROW_CAPACITY(list->capacity);
        void *memory = realloc(list->patches, sizeof(FunctionId) * list->capacity);
        if (memory != NULL) {
            list->patches = (FilePatch *) memory;
        } else {
            fprintf(stderr, "Memory error.\n");
            exit(SAVE_FAILURE);
        }
    }
    list->patches[list->count] = id;
    list->count++;
}


// When all functions are outputed go thorugh the fix list and write the proper id's

static void write_int(FILE *out, int num) {
    if (NULL == out) {
        fprintf(stderr, "File was NULL.\n");
        exit(SAVE_FAILURE);
    }
    fwrite(&num, sizeof(int), 1, out);
    if (ferror(out)) {
        perror(__func__);
        exit(SAVE_FAILURE);
    }
}

static void write_byte(FILE *out, uint8_t num) {
    if (NULL == out) {
        fprintf(stderr, "File was NULL.\n");
        exit(SAVE_FAILURE);
    }
    fwrite(&num, sizeof(uint8_t), 1, out);
    if (ferror(out)) {
        perror(__func__);
        exit(SAVE_FAILURE);
    }
}

static void write_double(FILE *out, double num) {
    if (NULL == out) {
        fprintf(stderr, "File was NULL.\n");
        exit(SAVE_FAILURE);
    }
    fwrite(&num, sizeof(double), 1, out);
    if (ferror(out)) {
        perror(__func__);
        exit(SAVE_FAILURE);
    }
}

static void write_checked(FILE *f, const void *ptr, size_t size, size_t n) {
    if (NULL == f) {
        fprintf(stderr, "I bet you saw THAT coming.\n");
        exit(SAVE_FAILURE);
    }
    fwrite(ptr, size, n, f);
    if (ferror(f)) {
        perror(__func__);
        exit(SAVE_FAILURE);
    }
}

static void write_string(FILE *out, ObjString *string) {
    write_int(out, string->length);
    write_checked(out, string->chars, sizeof(char), string->length);
}

static void writeFunctionHeader(FILE *file, ObjFunction *function) {
    write_int(file, SEG_FUNCTION_HEADER);
    if (function->name) {
        write_int(file, SEG_FUNCTION_NAME);
        write_string(file, function->name);
    } else {
        write_int(file, SEG_FUNCTION_SCRIPT);
    }
    write_int(file, function->arity);
    write_int(file, function->upvalueCount);
}

static void writeFunctionCode(FILE *file, ObjFunction *function) {
    write_int(file, SEG_FUNCTION_CODE);

    write_int(file, function->chunk.count);
    write_int(file, function->chunk.capacity);
    write_checked(file, function->chunk.code, sizeof(uint8_t), function->chunk.count);

    write_int(file, function->chunk.lineCount);
    write_int(file, function->chunk.lineCapacity);
    write_checked(file, function->chunk.lines, sizeof(LineStart), function->chunk.lineCount);
}

static void writeFunctionConstants(FILE *file, ObjFunction *function,
                                   PatchList *patchList, ValueQueue *queue) {
    write_int(file, SEG_FUNCTION_CONSTANTS);
    ValueArray *constants = &function->chunk.constants;
    write_int(file, constants->count);
    for (int i = 0; i < constants->count; i++) {
        Value value = constants->values[i];
        if (IS_NUMBER(value)) {
            write_byte(file, OUT_TAG_NUMBER);
            write_double(file, AS_NUMBER(value));
        } else if (IS_STRING(value)) {
            write_byte(file, OUT_TAG_STRING);
            write_string(file, AS_STRING(value));
            // Saving like this might mess with string interning
            // Some strings might be read and allocated multiple times!
        } else if (IS_FUNCTION(value)) {
            if (!findInQueue(queue, value)) {
                enqueueValue(queue, value);
            }
            const char *const name = AS_FUNCTION(value)->name != NULL
                                     ? AS_FUNCTION(value)->name->chars
                                     : "<script>";
            write_byte(file, OUT_TAG_FUNCTION);
            addPatch(patchList, (FilePatch) {
                    .function = AS_FUNCTION(value),
                    .position = ftell(file)
            });
            write_int(file, 0x7FFFFFFF);
        } else {
            fprintf(stderr, "Invalid value type in constant array.");
            exit(SAVE_FAILURE);
        }
    }
}

static void writeFunction(FILE *file, ObjFunction *function,
                          PatchList *patchList, ValueQueue *queue) {
    write_int(file, SEG_FUNCTION);
    writeFunctionHeader(file, function);
    writeFunctionCode(file, function);
    writeFunctionConstants(file, function, patchList, queue);
    write_int(file, SEG_FUNCTION_END);
}

void outputToBinary(ObjFunction *compiled, const char *path) {
    FILE *file = fopen(path, "w+b");
    if (file == NULL) {
        fprintf(stderr, "File does not exist");
        exit(SAVE_FAILURE);
    }
    ValueQueue queue;
    initValueQueue(&queue);

    FunctionIdList ids;
    initFunctionIdList(&ids);

    PatchList patchList;
    initPatchList(&patchList);

    int id = 0;
    addFunctionId(&ids, (FunctionId) {
            .id = id++,
            .function = compiled
    });
    writeFunction(file, compiled, &patchList, &queue);
    while (queue.count > 0) {
        Value value = pollValue(&queue);
        ObjFunction *function = AS_FUNCTION(value);
        addFunctionId(&ids, (FunctionId) {
                .id = id++,
                .function = function
        });
        writeFunction(file, function, &patchList, &queue);
    }

    // iterate and fix patches
    for (int i = 0; i < patchList.count; i++) {
        FilePatch *patch = &patchList.patches[i];
        FunctionId *fid = findFunctionId(&ids, patch->function);
        if (fid == NULL) {
            fprintf(stderr, "Found a patch for non existent function.");
            exit(SAVE_FAILURE);
        }
        fseek(file, patch->position, SEEK_SET);
        write_int(file, fid->id);
    }
    fseek(file, 0L, SEEK_END);
    write_int(file, SEG_FILE_END);

    freeValueQueue(&queue);
    freeFunctionIdList(&ids);
    freePatchList(&patchList);

    fclose(file);
}

#undef SAVE_FAILURE