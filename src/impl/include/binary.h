#ifndef __CLOX2_BINARY_H__
#define __CLOX2_BINARY_H__

#include "object.h"

void writeBinary(ObjFunction* compiled, const char* path);

ObjFunction* loadBinary(const char* path);


#endif // __CLOX2_BINARY_H__
