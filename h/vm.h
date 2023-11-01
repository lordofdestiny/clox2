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

typedef struct {
    Obj *function;
    uint8_t *ip;
    Value *slots;
} CallFrame;

typedef struct {
    CallFrame frames[FRAMES_MAX];
    int frameCount;

    Chunk *chunk;
    Value stack[STACK_MAX];
    Value *stackTop;
    Table globals;
    Table strings;
    ObjUpvalue *openUpvalues;

    size_t bytesAllocated;
    size_t nextGC;
    Obj *objects;

    int grayCount;
    int grayCapacity;
    Obj **grayStack;
} VM;

typedef enum {
    INTERPRETER_OK,
    INTERPRETER_COMPILE_ERROR,
    INTERPRETER_RUNTIME_ERROR,
} InterpretResult;

extern VM vm;

void initVM();

void freeVM();

InterpretResult interpret(const char *source);

void push(Value value);

Value pop();

void runtimeError(const char *format, ...);

bool callNonNative(Obj *callee, ObjFunction *function, int argCount);

#endif //CLOX2_VM_H
