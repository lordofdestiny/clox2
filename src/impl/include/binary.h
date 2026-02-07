#ifndef __CLOX2_BINARY_H__
#define __CLOX2_BINARY_H__

#include <cloximpl_export.h>

#include "object.h"

CLOXIMPL_EXPORT void writeBinary(const char* source_file, ObjFunction* compiled, const char* path);

CLOXIMPL_EXPORT ObjFunction* loadBinary(const char* path);

#endif // __CLOX2_BINARY_H__
