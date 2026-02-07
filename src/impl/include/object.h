#ifndef __CLOX2_OBJECT_H__
#define __CLOX2_OBJECT_H__

#include <stdio.h>

#include "memory.h"
#include "value.h"
#include "chunk.h"
#include "table.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_ARRAY(value) isObjType(value, OBJ_ARRAY)
#define IS_BOUND_METHOD(value) isObjType(value, OBJ_BOUND_METHOD)
#define IS_CLASS(value) isObjType(value, OBJ_CLASS)
#define IS_CLOSURE(value) isObjType(value, OBJ_CLOSURE)
#define IS_FUNCTION(value) isObjType(value, OBJ_FUNCTION)
#define IS_INSTANCE(value) isObjType(value, OBJ_INSTANCE)
#define IS_NATIVE(value) isObjType(value, OBJ_NATIVE)
#define IS_STRING(value) isObjType(value, OBJ_STRING)

#define AS_ARRAY(value) ((ObjArray*) AS_OBJ(value))
#define AS_BOUND_METHOD(value) ((ObjBoundMethod*)AS_OBJ(value))
#define AS_CLASS(value) ((ObjClass*)AS_OBJ(value))
#define AS_CLOSURE(value) ((ObjClosure*)AS_OBJ(value))
#define AS_FUNCTION(value) ((ObjFunction *) AS_OBJ(value))
#define AS_INSTANCE(value) ((ObjInstance*) AS_OBJ(value))
#define AS_NATIVE(value) ((ObjNative*) AS_OBJ(value))
#define AS_STRING(value) ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)

#define CALL_OBJ(callee, argCount) AS_OBJ(callee)->vtp->call(AS_OBJ(callee), argCount)

#define OBJECT_TYPE_ENUM_LIST \
    ENUM_OBJTYPE_DEF(ARRAY) \
    ENUM_OBJTYPE_DEF(BOUND_METHOD) \
    ENUM_OBJTYPE_DEF(CLASS) \
    ENUM_OBJTYPE_DEF(CLOSURE) \
    ENUM_OBJTYPE_DEF(FUNCTION) \
    ENUM_OBJTYPE_DEF(INSTANCE) \
    ENUM_OBJTYPE_DEF(NATIVE) \
    ENUM_OBJTYPE_DEF(STRING) \
    ENUM_OBJTYPE_DEF(UPVALUE)

typedef enum {
#define ENUM_OBJTYPE_DEF(name) OBJ_##name,
OBJECT_TYPE_ENUM_LIST
#undef ENUM_OBJTYPE_DEF
} ObjType;

static inline const char* objTypeToString(const ObjType type) {
    switch (type) {
#define ENUM_OBJTYPE_DEF(name) \
        case OBJ_##name: return "OBJ_"#name;
        OBJECT_TYPE_ENUM_LIST
#undef ENUM_OBJTYPE_DEF
        default: return "unknown object type";
    }
}

#undef OBJECT_TYPE_ENUM_LIST

typedef bool (*CallableFn)(Obj*, int);
typedef void (*PrintFn)(Obj*, FILE* out);
typedef void (*BlackenFn)(Obj*);
typedef void (*FreeFn)(Obj*);

typedef struct {
    CallableFn call;
    PrintFn print;
    BlackenFn blacken;
    FreeFn free;
} ObjVT;

struct Obj {
    GcNode gcNode;
    ObjVT* vtp;
    ObjType type;
};

typedef struct {
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

typedef struct {
    Obj obj;
    ObjFunction* function;
    ObjUpvalue** upvalues;
    int upvalueCount;
} ObjClosure;

typedef bool (*NativeFn)(int argCount, Value* implicit, Value* args);

typedef struct {
    Obj obj;
    int arity;
    NativeFn function;
} ObjNative;

struct ObjString {
    Obj obj;
    int length;
    uint32_t hash;
    char* chars;
};

typedef struct {
    Obj obj;
    ObjString* name;
    Value initializer;
    Table fields;
    Table methods;
    Table staticMethods;
} ObjClass;

typedef struct {
    Obj obj;
    Value this_;
    ObjClass* klass;
    Table fields;
} ObjInstance;

typedef struct {
    Obj obj;
    Value receiver;
    Obj* method;
} ObjBoundMethod;

ObjArray* newArray();

ObjBoundMethod* newBoundMethod(Value receiver, Obj* method);

ObjClass* newClass(ObjString* name);

ObjClosure* newClosure(ObjFunction* function);

ObjFunction* newFunction();

ObjInstance* newInstance(ObjClass* klass);

ObjInstance* newPrimitive(Value value, ObjClass* klass);

ObjNative* newNative(NativeFn function, int arity);

ObjString* takeString(char* chars, int length);

ObjString* escapedString(const char* chars, int length);

ObjString* copyString(const char* chars, int length);

ObjUpvalue* newUpvalue(Value* slot);

void markObject(Obj* object);

void printObject(FILE* out, Value value);

static inline bool isObjType(const Value value, const ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif //__CLOX2_OBJECT_H__
