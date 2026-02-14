#ifndef __CLOX2_VM_H__
#define __CLOX2_VM_H__

#include <setjmp.h>

#include <clox/export.h>

#include <clox/value.h>
#include <impl/chunk.h>
#include <impl/common.h>
#include <impl/object.h>

#include <clox/table.h>

#include <common/inputfile.h>

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)
#define MAX_HANDLER_FRAMES 16
#define MAX_NATIVE_RC 64

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
    const char* name;
    void* handle;
    LibraryEventFn onLoad;
    LibraryEventFn onUnload;
} NativeLibrary;

typedef struct {
    size_t nativeLibCount;
    NativeLibrary* nativeLibHandles;

    int nativeRcNext;
    Value nativeRc[MAX_NATIVE_RC];

    int nativeArgsCap;
    Value* nativeArgs;
} NativeLibraryState;

extern NativeLibraryState nativeState;

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
    bool exit_state_ready;
    int exit_code;
} VM;

extern VM vm;

typedef enum {
    INTERPRET_OK,
    INTERPRET_EXIT,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} InterpretResult;

CLOX_EXPORT void initVM();

CLOX_EXPORT void freeVM();

CLOX_EXPORT int vmExitCode();

void push(Value value);

Value pop();

CLOX_EXPORT InterpretResult interpret(InputFile source);

CLOX_EXPORT InterpretResult interpretCompiled(ObjFunction* function);

void runtimeError(const char* format, ...);

bool callClass(Obj* callable, int argCount);

bool callClosure(Obj* callable, int argCount);

bool callFunction(Obj* callable, int argCount);

bool callNative(Obj* callable, int argCount);

bool callBoundMethod(Obj* callable, int argCount);

#endif //__CLOX2_VM_H__
