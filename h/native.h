//
// Created by djumi on 11/10/2023.
//

#ifndef CLOX2_NATIVE_H
#define CLOX2_NATIVE_H

#include "common.h"
#include "value.h"
#include "object.h"

#define NATIVE_ERROR(msg) (OBJ_VAL((Obj*) copyString(msg, strlen(msg))))

typedef struct {
    const char *name;
    const int arity;
    const NativeFn function;
} NativeMethodDef;

extern NativeMethodDef nativeMethods[];

bool initExceptionNative(int argCount, Value *args);

bool initNumberNative(int argCount, Value *args);

bool initBooleanNative(int argCount, Value *args);

bool initStringNative(int argCount, Value *args);

bool initArrayNative(int argCount, Value *args);
bool appendArrayNative(int argCount, Value *args);
bool popArrayNative(int argCount, Value *args);

bool toPrecisionNative(int argCount, Value *args);

#endif //CLOX2_NATIVE_H
