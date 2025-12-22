#ifndef CLOX2_DEBUG_H
#define CLOX2_DEBUG_H

#include <stdio.h>
#include "chunk.h"

void disassembleChunk(FILE* file, Chunk* chunk, const char* name);
int disassembleInstruction(FILE* file, Chunk* chunk, int offset);

#endif //CLOX2_DEBUG_H
