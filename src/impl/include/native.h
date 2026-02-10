#ifndef __CLOX2_NATIVE_H__
#define __CLOX2_NATIVE_H__

#include "clox/value.h"
#include "clox/object.h"
#include "clox/native.h"

typedef struct {
    const char* name;
    const int arity;
    const NativeFn function;
} NativeMethodDef;

extern NativeMethodDef nativeMethods[];

bool initExceptionNative(int argCount, Value* implicit, Value* args);

bool initNumberNative(int argCount, Value* implicit, Value* args);

bool initBooleanNative(int argCount, Value* implicit, Value* args);

bool initStringNative(int argCount, Value* implicit, Value* args);

bool initArrayNative(int argCount, Value* implicit, Value* args);

bool appendArrayNative(int argCount, Value* implicit, Value* args);

bool popArrayNative(int argCount, Value* implicit, Value* args);

bool toPrecisionNative(int argCount, Value* implicit, Value* args);

#endif //__CLOX2_NATIVE_H__
