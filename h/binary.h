//
// Created by djumi on 11/20/2023.
//

#ifndef CLOX2_BINARY_H
#define CLOX2_BINARY_H

#include "object.h"

void writeBinary(ObjFunction *compiled, const char *path);

ObjFunction *loadBinary(const char *path);


#endif //CLOX2_BINARY_H
