//
// Created by djumi on 11/20/2023.
//
#include <stdio.h>
#include <stdlib.h>

#include "../h/binary.h"
#include "../h/memory.h"
#include "../h/common.h"

#define SAVE_FAILURE 44

#define LOAD_ARRAY(type, file, dest, count) read_array(file, dest, sizeof(type), count)

typedef enum {
    SEG_FUNCTION = 0xBEEF,
    SEG_FUNCTION_HEADER,
    SEG_FUNCTION_NAME,
    SEG_FUNCTION_CODE,
    SEG_FUNCTION_CONSTANTS,
    SEG_FUNCTION_SCRIPT,
    SEG_FUNCTION_END,
    SEG_FILE_END = 0x7CADBEEF
} SegmentSequence;

typedef enum {
    OUT_TAG_NUMBER,
    OUT_TAG_STRING,
    OUT_TAG_FUNCTION
} ValueTag;

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
} FilePatchList;

static void initFilePatchList(FilePatchList *list) {
    list->count = 0;
    list->capacity = 0;
    list->patches = NULL;
}

static void freeFilePatchList(FilePatchList *list) {
    free(list->patches);
    initFilePatchList(list);
}

static void addFilePatch(FilePatchList *list, FilePatch id) {
    if (list->capacity < list->count + 1) {
        list->capacity = GROW_CAPACITY(list->capacity);
        void *memory = realloc(list->patches, sizeof(FilePatch) * list->capacity);
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
                                   FilePatchList *patchList, ValueQueue *queue) {
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
            write_byte(file, OUT_TAG_FUNCTION);
            addFilePatch(patchList, (FilePatch) {
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
                          FilePatchList *patchList, ValueQueue *queue) {
    write_int(file, SEG_FUNCTION);
    writeFunctionHeader(file, function);
    writeFunctionCode(file, function);
    writeFunctionConstants(file, function, patchList, queue);
    write_int(file, SEG_FUNCTION_END);
}

void patchFileRefs(FILE *file, FilePatchList *patchList, FunctionIdList *ids) {
    for (int i = 0; i < patchList->count; i++) {
        FilePatch *patch = &patchList->patches[i];
        FunctionId *fid = findFunctionId(ids, patch->function);
        if (fid == NULL) {
            fprintf(stderr, "Found a patch for non existent function.");
            exit(SAVE_FAILURE);
        }
        fseek(file, patch->position, SEEK_SET);
        write_int(file, fid->id);
    }
    fseek(file, 0L, SEEK_END);
}

void writeBinary(ObjFunction *compiled, const char *path) {
    FILE *file = fopen(path, "w+b");
    if (file == NULL) {
        fprintf(stderr, "File does not exist");
        exit(SAVE_FAILURE);
    }
    ValueQueue queue;
    initValueQueue(&queue);

    FunctionIdList ids;
    initFunctionIdList(&ids);

    FilePatchList patchList;
    initFilePatchList(&patchList);

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

    patchFileRefs(file, &patchList, &ids);
    write_int(file, SEG_FILE_END);

    freeValueQueue(&queue);
    freeFunctionIdList(&ids);
    freeFilePatchList(&patchList);

    fclose(file);
}

#undef SAVE_FAILURE

#define LOAD_FAILURE 33

#ifdef DEBUG_PRINT_CODE

#include "../h/debug.h"

#endif

typedef struct {
    int toPatch; // Function to patch
    int patchWith; // Function to patch with
    int position; // Index in constants array
} FunctionPatch;

typedef struct {
    int count;
    int capacity;
    FunctionPatch *patches;
} FunctionPatchList;

static void initFunctionPatchList(FunctionPatchList *list) {
    list->count = 0;
    list->capacity = 0;
    list->patches = NULL;
}

static void freeFunctionPatchList(FunctionPatchList *list) {
    free(list->patches);
    initFunctionPatchList(list);
}

static void addFunctionPatch(FunctionPatchList *list, FunctionPatch patch) {
    if (list->capacity < list->count + 1) {
        list->capacity = GROW_CAPACITY(list->capacity);
        void *memory = realloc(list->patches, sizeof(FunctionPatch) * list->capacity);
        if (memory != NULL) {
            list->patches = (FunctionPatch *) memory;
        } else {
            fprintf(stderr, "memory error.\n");
            exit(LOAD_FAILURE);
        }
    }
    list->patches[list->count] = patch;
    list->count++;
}

static int read_int(FILE *file) {
    int value;
    if (fread(&value, sizeof(int), 1, file) != 1) {
        if (feof(file)) {
            fprintf(stderr, "Unexpected end of file.\n");
        } else if (ferror(file)) {
            perror(__func__);
        }
        exit(LOAD_FAILURE);
    }
    return value;
}


static int peek_int(FILE *file) {
    long location = ftell(file);
    int value = read_int(file);
    fseek(file, location - ftell(file), SEEK_CUR);
    return value;
}

static uint8_t read_byte(FILE *file) {
    uint8_t value;
    if (fread(&value, sizeof(uint8_t), 1, file) != 1) {
        if (feof(file)) {
            fprintf(stderr, "Unexpected end of file.\n");
        } else if (ferror(file)) {
            perror(__func__);
        }
        exit(LOAD_FAILURE);
    }
    return value;
}

static double read_double(FILE *file) {
    double value;
    if (fread(&value, sizeof(double), 1, file) != 1) {
        if (feof(file)) {
            fprintf(stderr, "Unexpected end of file.\n");
        } else if (ferror(file)) {
            perror(__func__);
        }
        exit(LOAD_FAILURE);
    }
    return value;
}

static void read_array(FILE *file, void *dest, size_t size, size_t count) {
    if (fread(dest, size, count, file) != count) {
        if (feof(file)) {
            fprintf(stderr, "Unexpected end of file.\n");
        } else if (ferror(file)) {
            perror(__func__);
        }
        exit(LOAD_FAILURE);
    }
}

static void checkSegment(FILE *file, SegmentSequence seg) {
    if (read_int(file) != seg) {
        fprintf(stderr, "Invalid file format");
        exit(LOAD_FAILURE);
    }
}

static ObjString *read_string(FILE *file) {
    int length = read_int(file);
    char *chars = ALLOCATE(char, length + 1);
    LOAD_ARRAY(char, file, chars, length);
    chars[length] = '\0';
    return takeString(chars, length);
}

static void loadFunctionHeader(FILE *file, ObjFunction *function) {
    checkSegment(file, SEG_FUNCTION_HEADER);
    SegmentSequence seq = read_int(file);
    if (seq == SEG_FUNCTION_NAME) {
        function->name = read_string(file);
    } else if (seq == SEG_FUNCTION_SCRIPT) {
        function->name = NULL;
    } else {
        fprintf(stderr, "Unexpected sequence before function name.");
        exit(LOAD_FAILURE);
    }
    function->arity = read_int(file);
    function->upvalueCount = read_int(file);
}

static void loadFunctionCode(FILE *file, ObjFunction *function) {
    checkSegment(file, SEG_FUNCTION_CODE);
    Chunk *chunk = &function->chunk;

    chunk->count = read_int(file);
    chunk->capacity = read_int(file);
    chunk->code = ALLOCATE(uint8_t, chunk->capacity);
    LOAD_ARRAY(uint8_t, file, chunk->code, chunk->count);

    chunk->lineCount = read_int(file);
    chunk->lineCapacity = read_int(file);
    chunk->lines = ALLOCATE(LineStart, chunk->lineCapacity);
    LOAD_ARRAY(LineStart, file, chunk->lines, chunk->lineCount);
}

static void loadFunctionConstants(FILE *file, ObjFunction *function,
                                  int id, FunctionPatchList *patchList) {
    checkSegment(file, SEG_FUNCTION_CONSTANTS);
    ValueArray *constants = &function->chunk.constants;
    initValueArray(constants);
    int count = read_int(file);
    for (int i = 0; i < count; i++) {
        ValueTag tag = read_byte(file);
        if (tag == OUT_TAG_NUMBER) {
            double number = read_double(file);
            writeValueArray(constants, NUMBER_VAL(number));
        } else if (tag == OUT_TAG_STRING) {
            ObjString *string = read_string(file);
            writeValueArray(constants, OBJ_VAL((Obj *) string));
        } else if (tag == OUT_TAG_FUNCTION) {
            int missingFunction = read_int(file);
            addFunctionPatch(patchList, (FunctionPatch) {
                    .position = i,
                    .toPatch = id,
                    .patchWith = missingFunction
            });
            writeValueArray(constants, NIL_VAL);
        } else {
            fprintf(stderr, "Unexpected value tag. Found '%02X' at %08lX\n", tag, ftell(file) - 1);
            exit(LOAD_FAILURE);
        }
    }
}

static ObjFunction *loadFunction(FILE *file, int id,
                                 FunctionPatchList *patchList) {
    ObjFunction *function = newFunction();
    checkSegment(file, SEG_FUNCTION);
    loadFunctionHeader(file, function);
    loadFunctionCode(file, function);
    loadFunctionConstants(file, function, id, patchList);
    checkSegment(file, SEG_FUNCTION_END);
#ifdef DEBUG_PRINT_CODE
    disassembleChunk(&function->chunk,
                     function->name != NULL
                     ? function->name->chars
                     : "<script>");
#endif
    return function;
}

static void patchFunctionRefs(FunctionPatchList *patchList, ValueArray *functions) {
    for (int i = 0; i < patchList->count; i++) {
        FunctionPatch *patch = &patchList->patches[i];
        if (patch->patchWith > functions->count || patch->toPatch > functions->count) {
            fprintf(stderr, "Invalid function id to patch.");
            exit(LOAD_FAILURE);
        }
        ObjFunction *toPatch = AS_FUNCTION(functions->values[patch->toPatch]);
        ObjFunction *patchWith = AS_FUNCTION(functions->values[patch->patchWith]);
        toPatch->chunk.constants.values[patch->position] = OBJ_VAL((Obj *) patchWith);
    }
}

ObjFunction *loadBinary(const char *path) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }
    ValueArray functions;
    FunctionPatchList patchList;

    initValueArray(&functions);
    initFunctionPatchList(&patchList);

    int id = 0;
    ObjFunction *script = loadFunction(file, id++, &patchList);
    writeValueArray(&functions, OBJ_VAL(script));
    while (!feof(file) && peek_int(file) != SEG_FILE_END) {
        ObjFunction *function = loadFunction(file, id++, &patchList);
        writeValueArray(&functions, OBJ_VAL(function));
    }
    // Patch here
    patchFunctionRefs(&patchList, &functions);

    freeValueArray(&functions);
    freeFunctionPatchList(&patchList);

    fclose(file);
    return script;
}

#undef LOAD_FAILURE