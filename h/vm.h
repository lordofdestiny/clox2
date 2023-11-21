//
// Created by djumi on 10/29/2023.
//

#ifndef CLOX2_VM_H
#define CLOX2_VM_H

#include "chunk.h"
#include "value.h"
#include "table.h"
#include "object.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)
#define MAX_HANDLER_FRAMES 16

typedef struct {
    uint16_t handlerAddress;
    Value klass;
} ExceptionHandler;

typedef struct {
    Obj *function;
    uint8_t *ip;
    Value *slots;
    uint8_t handlerCount;
    ExceptionHandler handlerStack[MAX_HANDLER_FRAMES];
} CallFrame;

typedef struct {
    CallFrame frames[FRAMES_MAX];
    int frameCount;

    Chunk *chunk;
    Value stack[STACK_MAX];
    Value *stackTop;
    Table globals;
    Table strings;
    ObjString *initString;
    ObjUpvalue *openUpvalues;

    size_t bytesAllocated;
    size_t nextGC;
    Obj *objects;

    int grayCount;
    int grayCapacity;
    Obj **grayStack;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} InterpretResult;

extern VM vm;

void initVM();

void freeVM();

InterpretResult interpret(const char *source);

InterpretResult interpretCompiled(ObjFunction *function);

void push(Value value);

Value pop();

void runtimeError(const char *format, ...);

bool callClass(Obj *callable, int argCount);

bool callClosure(Obj *callable, int argCount);

bool callFunction(Obj *callable, int argCount);

bool callNative(Obj *callable, int argCount);

bool callBoundMethod(Obj *callable, int argCount);

#endif //CLOX2_VM_H
