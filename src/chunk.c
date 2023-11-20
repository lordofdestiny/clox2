//
// Created by djumi on 10/29/2023.
//

#include "../h/chunk.h"
#include "../h/memory.h"
#include "../h/value.h"
#include "../h/vm.h"

void initChunk(Chunk *chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;

    chunk->tryCatchBlockCount = 0;
    chunk->tryCatchBlockCapacity = 0;
    chunk->tryCatchBlocks = 0;

    chunk->lineCount = 0;
    chunk->lineCapacity = 0;
    chunk->lines = NULL;

    initValueArray(&chunk->constants);
}

void freeChunk(Chunk *chunk) {
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FREE_ARRAY(LineStart, chunk->lines, chunk->lineCapacity);
    freeValueArray(&chunk->constants);
    initChunk(chunk);
}

void writeChunk(Chunk *chunk, uint8_t byte, int line) {
    if (chunk->capacity < chunk->count + 1) {
        int oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->capacity);
    }

    chunk->code[chunk->count] = byte;
    chunk->count++;

    if (chunk->lineCount > 0 &&
        chunk->lines[chunk->lineCount - 1].line == line) {
        return;
    }

    if (chunk->lineCapacity < chunk->lineCount + 1) {
        int oldCapacity = chunk->lineCapacity;
        chunk->lineCapacity = GROW_CAPACITY(oldCapacity);
        chunk->lines = GROW_ARRAY(LineStart, chunk->lines, oldCapacity, chunk->lineCapacity);
    }

    LineStart *lineStart = &chunk->lines[chunk->lineCount++];
    lineStart->offset = chunk->count - 1;
    lineStart->line = line;
}

int getLine(Chunk *chunk, int instruction) {
    int start = 0;
    int end = chunk->lineCount - 1;
    while (true) {
        int mid = (start + end) / 2;
        LineStart *line = &chunk->lines[mid];

        if (instruction < line->offset) {
            end = mid - 1;
        } else if (mid == chunk->lineCount - 1 ||
                   instruction < chunk->lines[mid + 1].offset) {
            return line->line;
        } else {
            start = mid + 1;
        }
    }
}

int addConstant(Chunk *chunk, Value value) {
    push(value);
    writeValueArray(&chunk->constants, value);
    pop();
    return chunk->constants.count - 1;
}

void addTryCatchBlock(Chunk *chunk, int from, int to, int target) {
    if (chunk->tryCatchBlockCapacity < chunk->tryCatchBlockCount + 1) {
        int oldCapacity = chunk->tryCatchBlockCapacity;
        chunk->tryCatchBlockCapacity = oldCapacity + 2;
        chunk->tryCatchBlocks = GROW_ARRAY(
                TryCatchBlock, chunk->tryCatchBlocks,
                oldCapacity, chunk->tryCatchBlockCapacity);
    }
    chunk->tryCatchBlocks[chunk->tryCatchBlockCount] = (TryCatchBlock) {
            .from = from,
            .to = to,
            .target = target
    };
    chunk->tryCatchBlockCount++;
}