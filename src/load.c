//
// Created by djumi on 11/19/2023.
//
#include <stdio.h>
#include <stdlib.h>

#include "../h/load.h"
#include "../h/output.h"
#include "../h/memory.h"

#ifdef DEBUG_PRINT_CODE

#include "../h/debug.h"

#endif

#define LOAD_FAILURE 33
#define LOAD_ARRAY(type, file, dest, count) read_array(file, dest, sizeof(type), count)

typedef struct {
    int toPatch; // Function to patch
    int patchWith; // Function to patch with
    int position; // Index in constants array
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

static void addPatch(PatchList *list, FilePatch patch) {
    if (list->capacity < list->count + 1) {
        list->capacity = GROW_CAPACITY(list->capacity);
        void *memory = realloc(list->patches, sizeof(FilePatch) * list->capacity);
        if (memory != NULL) {
            list->patches = (FilePatch *) memory;
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
    int read;
    if ((read = read_int(file)) != seg) {
        fprintf(stderr, "Invalid file format. Read: %08X; Expected: %08X; Location: %lx;\n", read, seg, ftell(file));
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
                                  int id, PatchList *patchList) {
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
            addPatch(patchList, (FilePatch) {
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
                                 PatchList *patchList) {
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

ObjFunction *load(const char *path) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }
    ValueArray functions;
    initValueArray(&functions);
    PatchList patchList;
    initPatchList(&patchList);

    int id = 0;
    ObjFunction *script = loadFunction(file, id++, &patchList);
    writeValueArray(&functions, OBJ_VAL(script));
    while (!feof(file) && peek_int(file) != SEG_FILE_END) {
        ObjFunction *function = loadFunction(file, id++, &patchList);
        writeValueArray(&functions, OBJ_VAL(function));
    }
    // Patch here
    for (int i = 0; i < patchList.count; i++) {
        FilePatch *patch = &patchList.patches[i];
        if (patch->patchWith > functions.count || patch->toPatch > functions.count) {
            fprintf(stderr, "Invalid function id to patch.");
            exit(LOAD_FAILURE);
        }
        ObjFunction *toPatch = AS_FUNCTION(functions.values[patch->toPatch]);
        ObjFunction *patchWith = AS_FUNCTION(functions.values[patch->patchWith]);
        toPatch->chunk.constants.values[patch->position] = OBJ_VAL((Obj *) patchWith);
    }

    freeValueArray(&functions);
    freePatchList(&patchList);

    fclose(file);
    return script;
}


#undef LOAD_FAILURE