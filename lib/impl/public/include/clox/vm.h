#ifndef __CLOX_LIB_VM_H__
#define __CLOX_LIB_VM_H__

#include <clox/export.h>

#include <clox/value.h>
#include <clox/native.h>

typedef bool (*DefineNativeFunctionFn)(const char* name, int arity, NativeFn native);

CLOX_EXPORT
__attribute__((noreturn))
void terminate(int code);

CLOX_EXPORT void push(Value value);

CLOX_EXPORT Value pop();

#define PUSH_OBJ(obj) push(OBJ_VAL((Obj*) obj))

#endif //  __CLOX_LIB_VM_H__
