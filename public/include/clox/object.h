#ifndef __CLOX_LIB_OBJECT_H__
#define __CLOX_LIB_OBJECT_H__

#include <inttypes.h>
#include <stdio.h>

#include <clox_export.h>

#include "clox/native.h"
#include "clox/value.h"

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

typedef struct ObjVT ObjVT;

typedef struct Obj {
    ObjType type;
    bool isMarked;
    ObjVT* vtp;
    struct Obj* next;
} Obj;

typedef struct ObjArray ObjArray;

typedef struct ObjFunction ObjFunction;

typedef struct ObjUpvalue ObjUpvalue;

typedef struct ObjClosure ObjClosure;

typedef struct ObjNative ObjNative;

typedef struct ObjString ObjString;

typedef struct ObjClass ObjClass;

typedef struct ObjInstance ObjInstance;

typedef struct ObjBoundMethod ObjBoundMethod;

CLOX_EXPORT ObjArray* newArray();

CLOX_EXPORT ObjBoundMethod* newBoundMethod(Value receiver, Obj* method);

CLOX_EXPORT ObjClass* newClass(ObjString* name);

CLOX_EXPORT ObjClosure* newClosure(ObjFunction* function);

CLOX_EXPORT ObjFunction* newFunction();

CLOX_EXPORT ObjInstance* newInstance(ObjClass* klass);

CLOX_EXPORT ObjInstance* newPrimitive(Value value, ObjClass* klass);

CLOX_EXPORT ObjNative* newNative(NativeFn function, int arity);

CLOX_EXPORT ObjString* takeString(char* chars, int length);

CLOX_EXPORT ObjString* escapedString(const char* chars, int length);

CLOX_EXPORT ObjString* copyString(const char* chars, int length);

CLOX_EXPORT ObjUpvalue* newUpvalue(Value* slot);

CLOX_EXPORT uint32_t hashString(const char* chars, int length);

CLOX_EXPORT void printObject(FILE* out, Value value);

static inline bool isObjType(const Value value, const ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

/** 
 * TODO
 *
 * It allows us to implement some native methods without
 * having to provide per object type API'sj.
 * This will be removed in the future.
**/

#include "table.h"

typedef struct ObjClass {
    Obj obj;
    ObjString* name;
    Value initializer;
    Table fields;
    Table methods;
    Table staticMethods;
} ObjClass;

typedef struct ObjInstance {
    Obj obj;
    Value this_;
    ObjClass* klass;
    Table fields;
} ObjInstance;

#endif // __CLOX_LIB_OBJECT_H__
