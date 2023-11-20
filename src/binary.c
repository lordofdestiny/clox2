//
// Created by djumi on 11/20/2023.
//
#include <stdio.h>
#include <stdlib.h>

#include "../h/binary.h"
#include "../h/memory.h"

#define SAVE_FAILURE 44

#define LOAD_ARRAY(type, file, dest, count) read_array(file, dest, sizeof(type), count)

#define INIT_GENERIC(type, array) initGenericArray(array, sizeof(type))
#define GET_ELEMENT(type, array, index) ((type*) getElement(array, index))

typedef enum {
    SEG_FUNCTIONS = 0xBEEF,
    SEG_FUNCTION,
    SEG_FUNCTION_HEADER,
    SEG_FUNCTION_NAME,
    SEG_FUNCTION_CODE,
    SEG_FUNCTION_CONSTANTS,
    SEG_FUNCTION_SCRIPT,
    SEG_FUNCTION_END,
    SEG_END_FUNCTIONS,
    SEG_STRINGS,
    SEG_END_STRINGS,
    SEG_FILE_END = 0x7CADBEEF
} SegmentSequence;

typedef enum {
    OUT_TAG_NUMBER,
    OUT_TAG_STRING,
    OUT_TAG_FUNCTION
} ValueTag;

typedef struct {
    Value *values;
    int bufferCount, bufferCapacity;
    int ringCount, ringCapacity;
    int head, tail;
} ValueQueue;

static void initValueQueue(ValueQueue *queue) {
    queue->values = 0;
    queue->bufferCount = 0;
    queue->bufferCapacity = 0;
    queue->ringCount = 0;
    queue->ringCapacity = 0;
    queue->head = 0;
    queue->tail = 0;
}

static void freeValueQueue(ValueQueue *queue) {
    free(queue->values);
    initValueQueue(queue);
}

static bool findInQueue(ValueQueue *queue, Value value) {
    for (int i = 0; i < queue->bufferCapacity; i++) {
        if (IS_NIL(queue->values[i])) continue;
        if (IS_NUMBER(value) && IS_NUMBER(queue->values[i]) &&
            AS_NUMBER(value) == AS_NUMBER(queue->values[i])) {
            return true;
        }
        if (value == queue->values[i]) return true;
    }
    return false;
}

static void enqueueValue(ValueQueue *queue, Value value) {
    bool expandBuffer = false;
    bool expandRing = false;
    if (queue->ringCapacity != queue->bufferCapacity) {
        expandBuffer = (queue->tail == 0);
        expandRing = false;
    } else if (queue->ringCapacity == queue->ringCount) {
        expandBuffer = true;
        expandRing = (queue->tail == 0);
    }

    if (expandBuffer) {
        queue->tail = queue->bufferCapacity;
        queue->bufferCapacity = GROW_CAPACITY(queue->bufferCapacity);
        void *memory = realloc(queue->values, sizeof(Value) * queue->bufferCapacity);
        if (memory != NULL) {
            queue->values = (Value *) memory;
            for (int i = queue->tail; i < queue->bufferCapacity; i++) {
                queue->values[i] = NIL_VAL;
            }
        } else {
            fprintf(stderr, "Memory error.\n");
            exit(SAVE_FAILURE);
        }
        if (expandRing) {
            queue->ringCapacity = queue->bufferCapacity;
        }
    }

    queue->values[queue->tail] = value;
    queue->bufferCount++;
    if (queue->tail < queue->ringCapacity) {
        queue->ringCount++;
    }
    queue->tail = (queue->tail + 1) % queue->bufferCapacity;
}

static Value pollValue(ValueQueue *queue) {
    Value value = queue->values[queue->head];
    queue->values[queue->head] = NIL_VAL;
    queue->bufferCount--;
    queue->ringCount--;
    if (queue->ringCount == 0 && queue->bufferCount != 0) {
        queue->head = queue->ringCapacity;
        queue->ringCount = queue->bufferCount;
        queue->ringCapacity = queue->bufferCapacity;
    } else {
        queue->head = (queue->head + 1) % queue->ringCapacity;
    }
    return value;
}

static bool queueEmpty(ValueQueue *queue) {
    return queue->bufferCount == 0;
}

typedef struct {
    int count;
    int capacity;
    size_t elementSize;
    char *elements;
} GenericArray;

static void initGenericArray(GenericArray *array, size_t size) {
    array->count = 0;
    array->capacity = 0;
    array->elementSize = size;
    array->elements = NULL;
}

static void freeGenericArray(GenericArray *array) {
    free(array->elements);
    initGenericArray(array, array->elementSize);
}

static void appendGenericArray(GenericArray *valueIds, void *item) {
    if (valueIds->capacity < valueIds->count + 1) {
        valueIds->capacity = GROW_CAPACITY(valueIds->capacity);
        void *memory = realloc(valueIds->elements, valueIds->elementSize * valueIds->capacity);
        if (memory != NULL) {
            valueIds->elements = memory;
        } else {
            fprintf(stderr, "Memory error.\n");
            exit(1);
        }
    }
    memcpy(valueIds->elements + valueIds->count * valueIds->elementSize, item, valueIds->elementSize);
    valueIds->count++;
}

static void *getElement(GenericArray *array, size_t index) {
    return array->elements + index * array->elementSize;
}

typedef struct {
    Value value;
    int id;
} ValueId;

static ValueId newFunctionValueId(Value value) {
    static int nextValueId = 0;
    return (ValueId) {
            .id = nextValueId++,
            .value = value
    };
}

static ValueId newStringValueId(Value value) {
    static int nextValueId = 0;
    return (ValueId) {
            .id = nextValueId++,
            .value = value
    };
}

static ValueId *findValueId(GenericArray *valueIds, Value value) {
    for (int i = 0; i < valueIds->count; i++) {
        ValueId *id = GET_ELEMENT(ValueId, valueIds, i);
        if (IS_NUMBER(value) && IS_NUMBER(id->value) &&
            AS_NUMBER(value) == AS_NUMBER(id->value)) {
            return id;
        }
        if (value == id->value) return id;
    }
    return false;
}

typedef struct {
    long position;
    Value value;
} FilePatch;

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
                                   GenericArray *valueIds, GenericArray *patchList,
                                   ValueQueue *functionQueue, ValueQueue *stringQueue) {
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
            if (!findInQueue(stringQueue, value)) {
                ValueId vid = newStringValueId(value);
                appendGenericArray(valueIds, &vid);
                enqueueValue(stringQueue, value);
            }
            appendGenericArray(patchList, &(FilePatch) {
                    .value = value,
                    .position = ftell(file)
            });
            write_int(file, 0x7FFFFFFF);
        } else if (IS_FUNCTION(value)) {
            if (!findInQueue(functionQueue, value)) {
                enqueueValue(functionQueue, value);
            }
            write_byte(file, OUT_TAG_FUNCTION);
            appendGenericArray(patchList, &(FilePatch) {
                    .value = value,
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
                          GenericArray *valueIds, GenericArray *patchList,
                          ValueQueue *functionQueue, ValueQueue *stringQueue) {
    ValueId vid = newFunctionValueId(OBJ_VAL(function));
    appendGenericArray(valueIds, &vid);

    write_int(file, SEG_FUNCTION);
    writeFunctionHeader(file, function);
    writeFunctionCode(file, function);
    writeFunctionConstants(file, function, valueIds, patchList, functionQueue, stringQueue);
    write_int(file, SEG_FUNCTION_END);
}

void patchFileRefs(FILE *file, GenericArray *patchList, GenericArray *valueIds) {
    for (int i = 0; i < patchList->count; i++) {
        FilePatch *patch = GET_ELEMENT(FilePatch, patchList, i);
        ValueId *fid = findValueId(valueIds, patch->value);
        if (fid == NULL) {
            fprintf(stderr, "Found a patch for non existent value.");
            exit(SAVE_FAILURE);
        }
        fseek(file, patch->position, SEEK_SET);
        write_int(file, fid->id);
    }
    fseek(file, 0L, SEEK_END);
}

void writeStrings(FILE *file, ValueQueue *strings) {
    write_int(file, SEG_STRINGS);
    while (!queueEmpty(strings)) {
        Value value = pollValue(strings);
        ObjString *string = AS_STRING(value);
        write_string(file, string);
    }
    write_int(file, SEG_END_STRINGS);
}

void writeBinary(ObjFunction *compiled, const char *path) {
    FILE *file = fopen(path, "w+b");
    if (file == NULL) {
        fprintf(stderr, "File does not exist");
        exit(SAVE_FAILURE);
    }
    ValueQueue functionQueue;
    initValueQueue(&functionQueue);

    ValueQueue stringQueue;
    initValueQueue(&stringQueue);

    GenericArray valueIds;
    INIT_GENERIC(ValueId, &valueIds);

    GenericArray patchList;
    INIT_GENERIC(FilePatch, &patchList);

    write_int(file, SEG_FUNCTIONS);
    writeFunction(file, compiled, &valueIds, &patchList, &functionQueue, &stringQueue);
    while (!queueEmpty(&functionQueue)) {
        Value value = pollValue(&functionQueue);
        ObjFunction *function = AS_FUNCTION(value);
        writeFunction(file, function, &valueIds, &patchList, &functionQueue, &stringQueue);
    }
    write_int(file, SEG_END_FUNCTIONS);

    writeStrings(file, &stringQueue);

    patchFileRefs(file, &patchList, &valueIds);
    write_int(file, SEG_FILE_END);

    freeValueQueue(&functionQueue);
    freeGenericArray(&valueIds);
    freeGenericArray(&patchList);

    fclose(file);
}

#define LOAD_FAILURE 33

#ifdef DEBUG_PRINT_CODE

#include "../h/debug.h"

#endif

typedef struct {
    int toPatch; // Function to patch
    int patchWith; // Value to patch with
    int position; // Index in constants array
    ValueTag type;
} FunctionPatch;

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
    int read;
    if ((read = read_int(file)) != seg) {
        fprintf(stderr, "Invalid file format. Read: %08X; Expected: %08X", read, seg);
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
                                  GenericArray *patchList) {
    static int nextFunctionId = 0;
    int id = nextFunctionId++;

    checkSegment(file, SEG_FUNCTION_CONSTANTS);
    ValueArray *constants = &function->chunk.constants;
    initValueArray(constants);
    int count = read_int(file);
    for (int i = 0; i < count; i++) {
        ValueTag tag = read_byte(file);
        switch (tag) {
        case OUT_TAG_NUMBER: {
            double number = read_double(file);
            writeValueArray(constants, NUMBER_VAL(number));
            break;
        }
        case OUT_TAG_STRING:
        case OUT_TAG_FUNCTION: {
            int missingValue = read_int(file);
            appendGenericArray(patchList, &(FunctionPatch) {
                    .position = i,
                    .toPatch = id,
                    .type = tag,
                    .patchWith = missingValue
            });
            writeValueArray(constants, NIL_VAL);
            break;
        }
        default: {
            fprintf(stderr, "Unexpected value tag. Found '%02X' at %08lX\n", tag, ftell(file) - 1);
            exit(LOAD_FAILURE);
        }
        }
    }
}

static ObjFunction *loadFunction(FILE *file, GenericArray *patchList) {
    ObjFunction *function = newFunction();
    checkSegment(file, SEG_FUNCTION);
    loadFunctionHeader(file, function);
    loadFunctionCode(file, function);
    loadFunctionConstants(file, function, patchList);
    checkSegment(file, SEG_FUNCTION_END);
    return function;
}

static GenericArray loadStrings(FILE *file) {
    GenericArray strings;
    INIT_GENERIC(Value, &strings);
    checkSegment(file, SEG_STRINGS);
    while (!feof(file) && peek_int(file) != SEG_END_STRINGS) {
        ObjString *string = read_string(file);
        appendGenericArray(&strings, &(Value) {OBJ_VAL(string)});
    }
    checkSegment(file, SEG_END_STRINGS);
    return strings;
}

static void patchFunctionRefs(GenericArray *patchList, GenericArray *functions, GenericArray *strings) {
    for (int i = 0; i < patchList->count; i++) {
        FunctionPatch *patch = GET_ELEMENT(FunctionPatch, patchList, i);

        if (patch->type == OUT_TAG_FUNCTION) {
            if (patch->patchWith > functions->count || patch->toPatch > functions->count) {
                fprintf(stderr, "Invalid function id to patch.");
                exit(LOAD_FAILURE);
            }
            Value *toPatch = GET_ELEMENT(Value, functions, patch->toPatch);
            ObjFunction *toPatchFn = AS_FUNCTION(*toPatch);
            Value patchWith = *GET_ELEMENT(Value, functions, patch->patchWith);
            toPatchFn->chunk.constants.values[patch->position] = patchWith;
        } else if (patch->type == OUT_TAG_STRING) {
            if (patch->patchWith > strings->count || patch->toPatch > strings->count) {
                fprintf(stderr, "Invalid string id to patch.");
                exit(LOAD_FAILURE);
            }
            Value *toPatch = GET_ELEMENT(Value, functions, patch->toPatch);
            ObjFunction *toPatchFn = AS_FUNCTION(*toPatch);
            Value patchWith = *GET_ELEMENT(Value, strings, patch->patchWith);
            toPatchFn->chunk.constants.values[patch->position] = patchWith;
        }
    }
}

ObjFunction *loadBinary(const char *path) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }
    GenericArray functions;
    INIT_GENERIC(Value, &functions);

    GenericArray patchList;
    INIT_GENERIC(FunctionPatch, &patchList);

    checkSegment(file, SEG_FUNCTIONS);
    ObjFunction *script = loadFunction(file, &patchList);
    appendGenericArray(&functions, &(Value) {OBJ_VAL(script)});
    while (!feof(file) && peek_int(file) != SEG_END_FUNCTIONS) {
        ObjFunction *function = loadFunction(file, &patchList);
        appendGenericArray(&functions, &(Value) {OBJ_VAL(function)});
    }
    checkSegment(file, SEG_END_FUNCTIONS);

    GenericArray strings = loadStrings(file);

    patchFunctionRefs(&patchList, &functions, &strings);

//#ifdef DEBUG_PRINT_CODE
//    disassembleChunk(&function->chunk,
//                     function->name != NULL
//                     ? function->name->chars
//                     : "<script>");
//#endif

    freeGenericArray(&functions);
    freeGenericArray(&patchList);

    fclose(file);
    return script;
}