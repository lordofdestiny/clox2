#ifndef __CLOX2_COMPILER_H__
#define __CLOX2_COMPILER_H__

#include <stdbool.h>

#include "object.h"
#include "inputfile.h"

bool isRepl();
void setRepl(bool isRepl);

ObjFunction* compile(InputFile source);

void markCompilerRoots();

#endif //__CLOX2_COMPILER_H__
