#ifndef __CLOX2_COMPILER_H__
#define __CLOX2_COMPILER_H__

#include "vm.h"
#include "object.h"

ObjFunction* compile(const char* source);

void markCompilerRoots();

#endif //__CLOX2_COMPILER_H__
