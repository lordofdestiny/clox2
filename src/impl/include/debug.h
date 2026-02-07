#ifndef __CLOX2_DEBUG_H__
#define __CLOX2_DEBUG_H__

#include <stdio.h>
#include "chunk.h"

void disassembleChunk(FILE* file, Chunk* chunk, const char* name);

int disassembleInstruction(FILE* file, Chunk* chunk, int offset);

#endif //__CLOX2_DEBUG_H__
