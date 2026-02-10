#ifndef __CLOX2_OBJECT_H__
#define __CLOX2_OBJECT_H__

#include <stdio.h>

#include "clox/object.h"
#include "table.h"
#include "chunk.h"

#define CALL_OBJ(callee, argCount) AS_OBJ(callee)->vtp->call(AS_OBJ(callee), argCount)

typedef bool (*CallableFn)(Obj*, int);

typedef void (*BlackenFn)(Obj*);

typedef void (*FreeFn)(Obj*);

typedef void (*PrintFn)(Obj*, FILE* out);

typedef struct ObjVT {
    CallableFn call;
    BlackenFn blacken;
    FreeFn free;
    PrintFn print;
} ObjVT;

typedef struct ObjArray{
    Obj obj;
    ValueArray array;
} ObjArray;

typedef struct ObjFunction {
    Obj obj;
    int arity;
    int upvalueCount;
    Chunk chunk;
    ObjString* name;
} ObjFunction;

typedef struct ObjUpvalue {
    Obj obj;
    Value* location;
    Value closed;
    struct ObjUpvalue* next;
} ObjUpvalue;

typedef struct ObjClosure {
    Obj obj;
    ObjFunction* function;
    ObjUpvalue** upvalues;
    int upvalueCount;
} ObjClosure;

typedef struct ObjNative {
    Obj obj;
    int arity;
    NativeFn function;
} ObjNative;

typedef struct ObjString {
    Obj obj;
    int length;
    uint32_t hash;
    char* chars;
} ObjString;

typedef struct ObjBoundMethod {
    Obj obj;
    Value receiver;
    Obj* method;
} ObjBoundMethod;

#endif //__CLOX2_OBJECT_H__
