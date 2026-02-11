#ifndef __CLOX2_VM_H__
#define __CLOX2_VM_H__

#include <setjmp.h>

#include <clox_export.h>

#include "clox/value.h"
#include "chunk.h"
#include "common.h"
#include "table.h"
#include "object.h"
#include "inputfile.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)
#define MAX_HANDLER_FRAMES 16

typedef struct {
    Value klass;
    uint16_t handlerAddress;
    uint16_t finallyAddress;
} ExceptionHandler;

typedef struct {
    Obj* function;
    uint8_t* ip;
    Value* slots;
    uint8_t handlerCount;
    ExceptionHandler handlerStack[MAX_HANDLER_FRAMES];
} CallFrame;

typedef void (*LibraryEventFn)(void);

typedef struct {
    void* handle;
    LibraryEventFn onLoad;
    LibraryEventFn onUnload;
} NativeLibrary;

typedef struct {
    CallFrame frames[FRAMES_MAX];
    int frameCount;

    Chunk* chunk;
    Value stack[STACK_MAX];
    Value* stackTop;
    Table globals;
    Table strings;
    ObjString* initString;
    ObjUpvalue* openUpvalues;

    size_t bytesAllocated;
    size_t nextGC;
    Obj* objects;

    int grayCount;
    int grayCapacity;
    Obj** grayStack;
    jmp_buf exit_state;
    int exit_code;

    size_t nativeLibCount;
    NativeLibrary* nativeLibHandles;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_EXIT,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} InterpretResult;

extern VM vm;

CLOX_EXPORT void initVM();

CLOX_EXPORT void freeVM();

CLOX_EXPORT int vmExitCode();

CLOX_EXPORT InterpretResult interpret(InputFile source);

CLOX_EXPORT InterpretResult interpretCompiled(ObjFunction* function);

void runtimeError(const char* format, ...);

bool callClass(Obj* callable, int argCount);

bool callClosure(Obj* callable, int argCount);

bool callFunction(Obj* callable, int argCount);

bool callNative(Obj* callable, int argCount);

bool callBoundMethod(Obj* callable, int argCount);

#endif //__CLOX2_VM_H__
