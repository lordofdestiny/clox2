#ifndef __CLOX2_COMPILER_H__
#define __CLOX2_COMPILER_H__

#include <stdbool.h>

#include "object.h"

bool isRepl();
void setRepl(bool isRepl);

ObjFunction* compile(const char* source);

void markCompilerRoots();

#endif //__CLOX2_COMPILER_H__
