#ifndef __CLOX2_COMPILER_H__
#define __CLOX2_COMPILER_H__

#include <stdbool.h>

#include <clox/export.h>

#include <impl/object.h>
#include <common/inputfile.h>

CLOX_EXPORT bool isRepl();
CLOX_EXPORT void setRepl(bool isRepl);

CLOX_EXPORT ObjFunction* compile(InputFile source);

void markCompilerRoots();

#endif //__CLOX2_COMPILER_H__
