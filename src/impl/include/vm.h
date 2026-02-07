#ifndef __CLOX2_VM_H__
#define __CLOX2_VM_H__

#include <setjmp.h>

#include <cloximpl_export.h>

#include "chunk.h"
#include "value.h"
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

    jmp_buf exit_state;
    int exit_code;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_EXIT,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} InterpretResult;

extern VM vm;

CLOXIMPL_EXPORT void initVM();

CLOXIMPL_EXPORT void freeVM();

CLOXIMPL_EXPORT int vmExitCode();

CLOXIMPL_EXPORT InterpretResult interpret(InputFile source);

CLOXIMPL_EXPORT InterpretResult interpretCompiled(ObjFunction* function);

void push(Value value);

Value pop();

void runtimeError(const char* format, ...);

bool callClass(Obj* callable, int argCount);

bool callClosure(Obj* callable, int argCount);

bool callFunction(Obj* callable, int argCount);

bool callNative(Obj* callable, int argCount);

bool callBoundMethod(Obj* callable, int argCount);

#endif //__CLOX2_VM_H__
