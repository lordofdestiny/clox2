//
// Created by djumi on 10/29/2023.
//

#ifndef CLOX2_DEBUG_H
#define CLOX2_DEBUG_H

#include "chunk.h"

void disassembleChunk(Chunk* chunk, const char* name);
int disassembleInstruction(Chunk* chunk, int offset);

#endif //CLOX2_DEBUG_H
