#ifndef __CLOX_PUBLIC_RESULT_H__
#define __CLOX_PUBLIC_RESULT_H__

#include "clox/object.h"

typedef struct ValueResult {
    bool success;
    union {
        Value value;
        Value exception;
    };
}ValueResult;

typedef struct NumberResult {
    bool success;
    union {
        double value;
        Value exception;
    };
}NumberResult;

typedef struct BoolResult {
    bool success;
    union {
        bool value;
        Value exception;
    };
}BoolResult;

typedef struct NilResult {
    bool success;
    union {
        Value value;
        Value exception;
    };
}NilResult;

typedef struct ObjResult {
    bool success;
    union {
        Obj* value;
        Value exception;
    };
}ObjResult;

typedef struct ArrayResult {
    bool success;
    union {
        ObjArray* value;
        Value exception;
    };
}ArrayResult;

typedef struct ClassResult {
    bool success;
    union {
        ObjClass* value;
        Value exception;
    };
}ClassResult;

typedef struct InstanceResult {
    bool success;
    union {
        ObjInstance* value;
        Value exception;
    };
}InstanceResult;

typedef struct StringResult {
    bool success;
    union {
        ObjString* value;
        Value exception;
    };
}StringResult;

#endif
