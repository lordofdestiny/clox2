#ifndef __CLOX2_BINARY_H__
#define __CLOX2_BINARY_H__

#include <clox/export.h>

#include "object.h"

CLOX_EXPORT void writeBinary(const char* source_file, ObjFunction* compiled, const char* path);

CLOX_EXPORT ObjFunction* loadBinary(const char* path);

#endif // __CLOX2_BINARY_H__
