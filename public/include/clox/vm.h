#ifndef __CLOX_LIB_VM_H__
#define __CLOX_LIB_VM_H__

#include "clox/value.h"
#include "clox_export.h"

CLOX_EXPORT void push(Value value);

CLOX_EXPORT Value pop();

#endif //  __CLOX_LIB_VM_H__
