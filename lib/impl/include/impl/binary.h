#ifndef __CLOX2_BINARY_H__
#define __CLOX2_BINARY_H__

#include <clox/export.h>

#include <impl/object.h>

CLOX_EXPORT void writeBinary(const char* path, const char* source_file, ObjFunction* compiled);

CLOX_EXPORT ObjFunction* loadBinary(FILE* file);

#endif // __CLOX2_BINARY_H__
