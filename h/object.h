//
// Created by djumi on 10/29/2023.
//

#ifndef CLOX2_OBJECT_H
#define CLOX2_OBJECT_H

#include "common.h"
#include "value.h"
#include "chunk.h"
#include "table.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_BOUND_METHOD(value) isObjType(value, OBJ_BOUND_METHOD)
#define IS_CLASS(value) isObjType(value, OBJ_CLASS)
#define IS_CALLABLE(value) (IS_OBJ(value) && AS_OBJ(value)->isCallable)
#define IS_CLOSURE(value) isObjType(value, OBJ_CLOSURE)
#define IS_FUNCTION(value) isObjType(value, OBJ_FUNCTION)
#define IS_INSTANCE(value) isObjType(value, OBJ_INSTANCE)
#define IS_NATIVE(value) isObjType(value, OBJ_NATIVE)
#define IS_STRING(value) isObjType(value, OBJ_STRING)

#define AS_BOUND_METHOD(value) ((ObjBoundMethod*)AS_OBJ(value))
#define AS_CLASS(value) ((ObjClass*)AS_OBJ(value))
#define AS_CALLABLE(value) ((Callable *) AS_OBJ(value))
#define AS_CLOSURE(value) ((ObjClosure*)AS_OBJ(value))
#define AS_FUNCTION(value) ((ObjFunction *) AS_OBJ(value))
#define AS_INSTANCE(value) ((ObjInstance*) AS_OBJ(value))
#define AS_NATIVE(value) (((ObjNative*) AS_OBJ(value)))
#define AS_STRING(value) ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)

#define CALL_CALLABLE(callee, argCount) AS_CALLABLE(callee)->caller(AS_CALLABLE(callee), argCount)

typedef enum {
    OBJ_BOUND_METHOD,
    OBJ_CLASS,
    OBJ_CLOSURE,
    OBJ_FUNCTION,
    OBJ_INSTANCE,
    OBJ_NATIVE,
    OBJ_STRING,
    OBJ_UPVALUE,
} ObjType;

struct Obj {
    ObjType type;
    bool isCallable;
    bool isMarked;
    struct Obj *next;
};

typedef struct Callable Callable;

typedef bool (*CallableFn)(Callable *, int argCount);

struct Callable {
    Obj obj;
    CallableFn caller;
};

typedef struct ObjFunction {
    Callable obj;
    int arity;
    int upvalueCount;
    Chunk chunk;
    ObjString *name;
} ObjFunction;

typedef struct ObjUpvalue {
    Obj obj;
    Value *location;
    Value closed;
    struct ObjUpvalue *next;
} ObjUpvalue;

typedef struct {
    Callable obj;
    ObjFunction *function;
    ObjUpvalue **upvalues;
    int upvalueCount;
} ObjClosure;

typedef bool (*NativeFn)(int argCount, Value *args);

typedef struct {
    Callable obj;
    int arity;
    NativeFn function;
} ObjNative;

struct ObjString {
    Obj obj;
    int length;
    uint32_t hash;
    char *chars;
};

typedef struct {
    Callable obj;
    ObjString *name;
    Value initializer;
    Table methods;
} ObjClass;

typedef struct {
    Obj obj;
    ObjClass *klass;
    Table fields;
} ObjInstance;

typedef struct {
    Callable obj;
    Value receiver;
    Callable *method;
} ObjBoundMethod;

ObjBoundMethod *newBoundMethod(Value receiver, Callable *method);

ObjClass *newClass(ObjString *name);

ObjClosure *newClosure(ObjFunction *function);

ObjFunction *newFunction();

ObjInstance *newInstance(ObjClass *klass);

ObjNative *newNative(NativeFn function, int arity);

ObjString *takeString(char *chars, int length);

ObjString *copyString(const char *chars, int length);

ObjUpvalue *newUpvalue(Value *slot);

uint32_t hashString(const char *chars, int length);

void printObject(Value value);

static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif //CLOX2_OBJECT_H
