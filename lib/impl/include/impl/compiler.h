#ifndef __CLOX2_COMPILER_H__
#define __CLOX2_COMPILER_H__

#include <stdbool.h>

#include <clox/export.h>

#include <impl/object.h>
#include <common/inputfile.h>

CLOX_EXPORT ObjFunction* compileFile(const char* path);

CLOX_EXPORT ObjFunction* compilePrompt(const char* prompt);

void markCompilerRoots();

#endif //__CLOX2_COMPILER_H__
