#ifndef __CLOX_LIB_NATIVE_H__
#define __CLOX_LIB_NATIVE_H__

#include "clox/value.h"

typedef bool (*NativeFn)(int argCount, Value* implicit, Value* args);

#define NATIVE_ERROR(msg) (OBJ_VAL((Obj*) copyString(msg, strlen(msg))))

#endif //  __CLOX_LIB_NATIVE_H__
