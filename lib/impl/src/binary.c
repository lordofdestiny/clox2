#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#include <clox/vm.h>

#include <impl/binary.h>

#define SAVE_FAILURE 44
#define LOAD_FAILURE 33
#define MEMORY_FAILURE 22

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

#define WRITE_ARRAY(type, file, src, count) write_checked(file, src, sizeof(type), count)

#define LOAD_ARRAY(type, file, dest, count) read_array(file, dest, sizeof(type), count)

#define GENERIC_INIT(type, array) initGenericArray(array, sizeof(type))

#define GENERIC_GET(type, array, index) ((type*) ((array)->elements + (index) * (array)->elementSize))


typedef enum {
    SEG_FILE_START = 0x0000020B, // ZERO ZERO ('C'-'A') ('L'-'A')
    SEG_LOX_ID = 0x0E170000, // ('O' - 'A') ('X' - 'A') ZERO ZERO
    SEG_LOX_NAME = 0x636C6F78, // c l o x
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
    Value* values;
    int bufferCount, bufferCapacity;
    int ringCount, ringCapacity;
    int head, tail;
} ValueQueue;

static void initValueQueue(ValueQueue* queue) {
    queue->values = NULL;
    queue->bufferCount = 0;
    queue->bufferCapacity = 0;
    queue->ringCount = 0;
    queue->ringCapacity = 0;
    queue->head = 0;
    queue->tail = 0;
}

static void freeValueQueue(ValueQueue* queue) {
    free(queue->values);
    initValueQueue(queue);
}

static bool findInQueue(const ValueQueue* queue, const Value value) {
    for (int i = 0; i < queue->bufferCapacity; i++) {
        if (IS_NIL(queue->values[i])) continue;
        if (valuesEqual(value, queue->values[i])) return true;
    }
    return false;
}

static void enqueueValue(ValueQueue* queue, const Value value) {
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
        void * newQueue = realloc(queue->values,  queue->bufferCapacity * sizeof(*queue->values));
        if (newQueue == NULL) {
            fprintf(stderr, "Failed to allocate more memory!\n");
            exit(MEMORY_FAILURE);
        }
        queue->values = newQueue;

        for (int i = queue->tail; i < queue->bufferCapacity; i++) {
            queue->values[i] = NIL_VAL;
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

static Value pollValue(ValueQueue* queue) {
    const Value value = queue->values[queue->head];
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

static bool queueEmpty(const ValueQueue* queue) {
    return queue->bufferCount == 0;
}

typedef struct {
    int count;
    int capacity;
    size_t elementSize;
    char* elements;
} GenericArray;

static void initGenericArray(GenericArray* array, const size_t size) {
    array->elements = NULL;
    array->count = 0;
    array->capacity = 0;
    array->elementSize = size;
}

static void freeGenericArray(GenericArray* array) {
    free(array->elements);
    initGenericArray(array, array->elementSize);
}

static void appendGenericArray(GenericArray* valueIds, const void* item) {
    if (valueIds->capacity < valueIds->count + 1) {
        valueIds->capacity = GROW_CAPACITY(valueIds->capacity);
        int newSize = valueIds->elementSize * valueIds->capacity;
        void* newElements = realloc(valueIds->elements, newSize);
        if (newElements == NULL) {
            fprintf(stderr, "Failed to allocate more memory!\n");
            exit(MEMORY_FAILURE);
        }
        valueIds->elements = newElements;
    }
    void* dest = valueIds->elements + valueIds->count * valueIds->elementSize;
    memmove(dest, item, valueIds->elementSize);
    valueIds->count++;
}

typedef struct {
    Value value;
    int id;
} ValueId;

static ValueId newFunctionValueId(const Value value) {
    static int nextValueId = 0;
    return (ValueId){
        .id = nextValueId++,
        .value = value
    };
}

static ValueId newStringValueId(const Value value) {
    static int nextValueId = 0;
    return (ValueId){
        .id = nextValueId++,
        .value = value
    };
}

static ValueId* findValueId(GenericArray* valueIds, const Value value) {
    for (int i = 0; i < valueIds->count; i++) {
        ValueId* id = GENERIC_GET(ValueId, valueIds, i);
        if (valuesEqual(value, id->value)) return id;
    }
    return NULL;
}

typedef struct {
    long position;
    Value value;
} FilePatch;

static void write_checked(FILE* f, const void* ptr, const size_t size, const size_t n) {
    fwrite(ptr, size, n, f);
    if (ferror(f)) {
        perror("Failed to write to file");
        exit(SAVE_FAILURE);
    }
}

static void write_byte(FILE* out, const uint8_t num) {
    write_checked(out, &num, sizeof(num), 1);
}

static void write_int(FILE* out, const int num) {
    write_checked(out, &num, sizeof(num), 1);
}

static void write_double(FILE* out, const double num) {
    write_checked(out, &num, sizeof(num), 1);
}

static void write_string(FILE* out, const char* string) {
    int length = strlen(string);
    write_int(out, strlen(string));
    WRITE_ARRAY(char, out, string, length);
}

static void writeFunctionHeader(FILE* file, const ObjFunction* function) {
    if (function->name) {
        write_int(file, SEG_FUNCTION_NAME);
        write_string(file, function->name->chars);
    } else {
        write_int(file, SEG_FUNCTION_SCRIPT);
    }
    write_int(file, function->arity);
    write_int(file, function->upvalueCount);
}

static void writeFunctionCode(FILE* file, const ObjFunction* function) {
    write_int(file, function->chunk.count);
    write_int(file, function->chunk.capacity);
    WRITE_ARRAY(uint8_t, file, function->chunk.code, function->chunk.count);

    write_int(file, function->chunk.lineCount);
    write_int(file, function->chunk.lineCapacity);
    WRITE_ARRAY(LineStart, file, function->chunk.lines, function->chunk.lineCount);
}

static void writeFunctionConstants(
    FILE* file, const ObjFunction* function,
    GenericArray* valueIds, GenericArray* patchList,
    ValueQueue* functionQueue, ValueQueue* stringQueue
) {
    const ValueArray* constants = &function->chunk.constants;
    write_int(file, constants->count);
    for (int i = 0; i < constants->count; i++) {
        const Value value = constants->values[i];

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
            appendGenericArray(
                patchList, &(FilePatch){
                    .value = value,
                    .position = ftell(file)
                });
            write_int(file, 0x7FFFFFFF);
        } else if (IS_FUNCTION(value)) {
            if (!findInQueue(functionQueue, value)) {
                enqueueValue(functionQueue, value);
            }
            write_byte(file, OUT_TAG_FUNCTION);
            appendGenericArray(
                patchList, &(FilePatch){
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

static void writeFunction(
    FILE* file, ObjFunction* function,
    GenericArray* valueIds, GenericArray* patchList,
    ValueQueue* functionQueue, ValueQueue* stringQueue
) {
    const ValueId vid = newFunctionValueId(OBJ_VAL((Obj*) function));
    appendGenericArray(valueIds, &vid);

    write_int(file, SEG_FUNCTION);
    
    write_int(file, SEG_FUNCTION_HEADER);
    writeFunctionHeader(file, function);
    
    write_int(file, SEG_FUNCTION_CODE);
    writeFunctionCode(file, function);
    
    write_int(file, SEG_FUNCTION_CONSTANTS);
    writeFunctionConstants(file, function, valueIds, patchList, functionQueue, stringQueue);
    
    write_int(file, SEG_FUNCTION_END);
}

static void patchFileRefs(FILE* file, GenericArray* patchList, GenericArray* valueIds) {
    for (int i = 0; i < patchList->count; i++) {
        const FilePatch* patch = GENERIC_GET(FilePatch, patchList, i);
        const ValueId* fid = findValueId(valueIds, patch->value);
        if (fid == NULL) {
            fprintf(stderr, "Found a patch for non existent value.");
            exit(SAVE_FAILURE);
        }
        fseek(file, patch->position, SEEK_SET);
        write_int(file, fid->id);
    }
    fseek(file, 0L, SEEK_END);
}

static void writeStrings(FILE* file, ValueQueue* strings) {
    while (!queueEmpty(strings)) {
        const Value value = pollValue(strings);
        write_string(file, AS_STRING(value)->chars);
    }
}

void writeBinary(const char* source_file, ObjFunction* compiled, const char* path) {
    push(OBJ_VAL((Obj*) compiled));
    FILE* file = fopen(path, "w+b");
    setbuf(file, NULL);

    if (file == NULL) {
        fprintf(stderr, "File does not exist");
        exit(SAVE_FAILURE);
    }
    write_int(file, SEG_FILE_START);
    write_int(file, SEG_LOX_ID);
    write_int(file, SEG_LOX_NAME);
    write_string(file, source_file);

    ValueQueue functionQueue;
    initValueQueue(&functionQueue);

    ValueQueue stringQueue;
    initValueQueue(&stringQueue);

    GenericArray valueIds;
    GENERIC_INIT(ValueId, &valueIds);

    GenericArray patchList;
    GENERIC_INIT(FilePatch, &patchList);

    write_int(file, SEG_FUNCTIONS);
    writeFunction(file, compiled, &valueIds, &patchList, &functionQueue, &stringQueue);
    while (!queueEmpty(&functionQueue)) {
        const Value value = pollValue(&functionQueue);
        ObjFunction* function = AS_FUNCTION(value);
        writeFunction(file, function, &valueIds, &patchList, &functionQueue, &stringQueue);
    }
    write_int(file, SEG_END_FUNCTIONS);

    write_int(file, SEG_STRINGS);
    writeStrings(file, &stringQueue);
    write_int(file, SEG_END_STRINGS);

    patchFileRefs(file, &patchList, &valueIds);
    write_int(file, SEG_FILE_END);

    freeValueQueue(&stringQueue);
    freeValueQueue(&functionQueue);
    freeGenericArray(&valueIds);
    freeGenericArray(&patchList);

    fclose(file);
    pop();
}

typedef struct {
    int toPatch; // Function to patch
    int patchWith; // Value to patch with
    int position; // Index in constants array
    ValueTag type;
} FunctionPatch;

static void read_checked(FILE* file, void* dest, const size_t size, const size_t count) {
    if (fread(dest, size, count, file) != count) {
        if (feof(file)) {
            fprintf(stderr, "Unexpected end of file.\n");
        } else if (ferror(file)) {
            perror("Failed to read from file");
        }
        exit(LOAD_FAILURE);
    }
}

static uint8_t read_byte(FILE* file) {
    uint8_t value;
    read_checked(file, &value, sizeof(value), 1);
    return value;
}

static int read_int(FILE* file) {
    int value;
    read_checked(file, &value, sizeof(value), 1);
    return value;
}

static int peek_int(FILE* file) {
    const int value = read_int(file);
    fseek(file, -sizeof(value), SEEK_CUR);
    return value;
}

static double read_double(FILE* file) {
    double value;
    read_checked(file, &value, sizeof(value), 1);
    return value;
}

static void read_array(FILE* file, void* dest, const size_t size, const size_t count) {
    read_checked(file, dest, size, count);
}

static void checkSegment(FILE* file, const SegmentSequence seg) {
    int read;
    if ((read = read_int(file)) != (int) seg) {
        fprintf(stderr, "Invalid file format. Read: %08X; Expected: %08X\n", read, seg);
        exit(LOAD_FAILURE);
    }
}

static ObjString* read_string(FILE* file) {
    const int length = read_int(file);
    char* chars = malloc(length + 1);
    LOAD_ARRAY(char, file, chars, length);
    chars[length] = '\0';
    ObjString* str = copyString(chars, length);
    free(chars);
    return str;
}

static void loadFunctionHeader(FILE* file, ObjFunction* function) {
    const SegmentSequence seq = read_int(file);
    if (seq == SEG_FUNCTION_SCRIPT) {
        function->name = NULL;
    } else if (seq == SEG_FUNCTION_NAME) {
        function->name = read_string(file);
    } else {
        fprintf(stderr, "Unexpected sequence before function name.");
        exit(LOAD_FAILURE);
    }
    function->arity = read_int(file);
    function->upvalueCount = read_int(file);
}

static void loadFunctionCode(FILE* file, ObjFunction* function) {
    Chunk* chunk = &function->chunk;

    chunk->count = read_int(file);
    chunk->capacity = read_int(file);
    chunk->code = calloc(chunk->capacity, sizeof(uint8_t));
    LOAD_ARRAY(uint8_t, file, chunk->code, chunk->count);

    chunk->lineCount = read_int(file);
    chunk->lineCapacity = read_int(file);
    chunk->lines = calloc(chunk->lineCapacity, sizeof(LineStart));
    LOAD_ARRAY(LineStart, file, chunk->lines, chunk->lineCount);
}

static void loadFunctionConstants(
    FILE* file, ObjFunction* function,
    GenericArray* patchList
) {
    static int nextFunctionId = 0;
    const int id = nextFunctionId++;

    ValueArray* constants = &function->chunk.constants;
    initValueArray(constants);
    const int count = read_int(file);
    for (int i = 0; i < count; i++) {
        const ValueTag tag = read_byte(file);
        switch (tag) {
        case OUT_TAG_NUMBER: {
            const double number = read_double(file);
            writeValueArray(constants, NUMBER_VAL(number));
            break;
        }
        case OUT_TAG_STRING:
        case OUT_TAG_FUNCTION: {
            const int missingValue = read_int(file);
            FunctionPatch patch = {
                .position = i,
                .toPatch = id,
                .type = tag,
                .patchWith = missingValue
            };
            appendGenericArray(patchList, &patch);
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

static Value loadFunction(FILE* file, GenericArray* patchList) {
    if (feof(file) ) {
        return NIL_VAL;
    }

    if (peek_int(file) == SEG_END_FUNCTIONS) {
        return NIL_VAL;
    }

    checkSegment(file, SEG_FUNCTION);

    ObjFunction* function = newFunction();
    push(OBJ_VAL((Obj*) function));
    
    checkSegment(file, SEG_FUNCTION_HEADER);
    loadFunctionHeader(file, function);

    checkSegment(file, SEG_FUNCTION_CODE);
    loadFunctionCode(file, function);
    
    checkSegment(file, SEG_FUNCTION_CONSTANTS);
    loadFunctionConstants(file, function, patchList);

    checkSegment(file, SEG_FUNCTION_END);
    
    pop();
    return OBJ_VAL((Obj*) function);
}

static void loadFunctions(FILE* file, ObjArray* functions, GenericArray* patchList) {
    checkSegment(file, SEG_FUNCTIONS);
    while (true) {
        Value function = loadFunction(file, patchList);
        if (IS_NIL(function)) break;
        push(function);
        writeValueArray(&functions->array, function);
        pop();
    }
    checkSegment(file, SEG_END_FUNCTIONS);
}

static Value loadString(FILE* file) {
    if (feof(file)) {
        return NIL_VAL;
    }

    if (peek_int(file) == SEG_END_STRINGS) {
        return NIL_VAL;
    }

    ObjString* string = read_string(file);
    return OBJ_VAL((Obj*) string);
}

static void loadStrings(FILE* file, ObjArray* strings) {
    checkSegment(file, SEG_STRINGS);
    while (true) {
        Value string = loadString(file);
        if (IS_NIL(string)) break;
        push(string);
        writeValueArray(&strings->array, string);
        pop();
    }
    checkSegment(file, SEG_END_STRINGS);
}

static void patchFunctionRefs(
    GenericArray* patchList, ObjArray* functions, ObjArray* strings
) {
    for (int i = 0; i < patchList->count; i++) {
        const FunctionPatch* patch = GENERIC_GET(FunctionPatch, patchList, i);

        ObjArray* patchSource;
        switch (patch->type) {
        case OUT_TAG_STRING: patchSource = strings;
            break;
        case OUT_TAG_FUNCTION: patchSource = functions;
            break;
        default:
            fprintf(stderr, "Invalid patch type.");
            exit(LOAD_FAILURE);
        }

        if (patch->patchWith > patchSource->array.count || patch->toPatch > patchSource->array.count) {
            fprintf(stderr, "Invalid function id to patch.");
            exit(LOAD_FAILURE);
        }

        const Value toPatch = functions->array.values[patch->toPatch];
        const ObjFunction* toPatchFn = AS_FUNCTION(toPatch);
        const Value patchWith = patchSource->array.values[patch->patchWith];
        toPatchFn->chunk.constants.values[patch->position] = patchWith;
    }
}

ObjFunction* loadBinary(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file c\"%s\".\n", path);
        exit(74);
    }

    checkSegment(file, SEG_FILE_START);
    checkSegment(file, SEG_LOX_ID);

    checkSegment(file, SEG_LOX_NAME);
    const int file_name_len = read_int(file);
    char* file_name = calloc(file_name_len+1, 1);
    LOAD_ARRAY(char, file, file_name, file_name_len);
    printf("-- file_name: %s --\n", file_name);
    free(file_name);

    ObjArray* functions = newArray();
    push(OBJ_VAL((Obj*) functions));
    
    ObjArray* strings = newArray();
    push(OBJ_VAL((Obj*) strings));

    GenericArray patchList;
    GENERIC_INIT(FunctionPatch, &patchList);

    loadFunctions(file, functions, &patchList);
    loadStrings(file, strings);

    patchFunctionRefs(&patchList, functions, strings);

    fclose(file);
    
    Value scriptVal = functions->array.values[0];
    ObjFunction* script = (ObjFunction*) AS_OBJ(scriptVal);

    pop(); // Pop strings array
    pop(); // Pop functions array
    
    freeGenericArray(&patchList);
    
    return script;
}
