//
// Created by djumi on 11/10/2023.
//

#ifndef CLOX2_NATIVE_H
#define CLOX2_NATIVE_H

#include "common.h"
#include "value.h"
#include "object.h"

typedef struct {
    const char* name;
    const int arity;
    const NativeFn function;
} NativeMethodDef;

extern NativeMethodDef nativeMethods[];

#endif //CLOX2_NATIVE_H
