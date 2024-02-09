//
// Created by djumi on 10/29/2023.
//

#ifndef CLOX2_COMPILER_H
#define CLOX2_COMPILER_H

#include "vm.h"
#include "object.h"

ObjFunction* compile(const char* source);

void markCompilerRoots();

#endif //CLOX2_COMPILER_H
