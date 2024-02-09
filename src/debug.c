//
// Created by djumi on 10/29/2023.
//
#include <stdio.h>

#include "../h/object.h"
#include "../h/debug.h"

#include <sys/types.h>

void disassembleChunk(Chunk* chunk, const char* name) {
    printf("== %s ==\n", name);

    for (int offset = 0; offset < chunk->count;) {
        offset = disassembleInstruction(chunk, offset);
    }
}

static uint8_t read_byte(const Chunk* chunk, const int offset) {
    return chunk->code[offset];
}

static uint16_t read_short(const Chunk* chunk, const int offset) {
    return (chunk->code[offset] << 8) | chunk->code[offset + 1];
}

static int constantInstruction(char* name, const Chunk* chunk, const int offset) {
    const uint8_t constant = read_byte(chunk, offset + 1);
    printf("%-29s %4d '", name, constant);
    printValue(stdout, chunk->constants.values[constant]);
    printf("'\n");
    return offset + 2;
}

static int longOperandInstruction(const char* name, const Chunk* chunk, const int offset) {
    const uint16_t constant = read_short(chunk, offset + 1);
    printf("%-29s %4d\n", name, constant);
    return offset + 3;
}

static int invokeInstruction(const char* name, const Chunk* chunk, const int offset) {
    const uint8_t constant = read_byte(chunk, offset + 1);
    const uint8_t argCount = read_byte(chunk, offset + 2);
    printf("%-29s (%d args) %4d '", name, argCount, constant);
    printValue(stdout, chunk->constants.values[constant]);
    printf("'\n");
    return offset + 3;
}

static int simpleInstruction(const char* name, const int offset) {
    printf("%s\n", name);
    return offset + 1;
}

static int byteInstruction(const char* name, const Chunk* chunk, const int offset) {
    const uint8_t slot = read_byte(chunk, offset + 1);
    printf("%-29s %4d\n", name, slot);
    return offset + 2;
}

static int jumpInstruction(const char* name, const int sign, const Chunk* chunk, const int offset) {
    const uint16_t jump = read_short(chunk, offset + 1);
    printf("%-29s %4d -> %d\n", name, offset, offset + 3 + sign * jump);
    return offset + 3;
}

static int closureInstruction(const char* name, const Chunk* chunk, int offset) {
    offset++;
    const uint8_t constant = read_byte(chunk, offset++);
    printf("%-29s %4d ", name, constant);
    printValue(stdout, chunk->constants.values[constant]);
    printf("\n");

    const ObjFunction* function = AS_FUNCTION(chunk->constants.values[constant]);
    for (int j = 0; j < function->upvalueCount; j++) {
        const int isLocal = read_byte(chunk, offset++);
        const int index = read_byte(chunk, offset++);
        printf(
            "%04d\t|\t\t\t%s %d\n",
            offset - 2, isLocal ? "local" : "upvalue", index);
    }
    return offset;
}

static int exceptionHandlerInstruction(const char* name, const Chunk* chunk, const int offset) {
    const uint8_t type = chunk->code[offset + 1];

    const uint16_t handlerAddress = read_short(chunk, offset + 2);
    const uint16_t finallyAddress = read_short(chunk, offset + 4);

    if (finallyAddress != 0xFFFF) {
        printf("%-29s %4d -> %d, %d\n", name, type, handlerAddress, finallyAddress);
    } else {
        printf("%-29s %4d -> %d\n", name, type, handlerAddress);
    }
    return offset + 6;
}

int disassembleInstruction(Chunk* chunk, const int offset) {
    printf("%04d ", offset);
    const int line = getLine(chunk, offset);
    if (offset > 0 && line == getLine(chunk, offset - 1)) {
        printf("\t| ");
    } else {
        printf("%4d ", line);
    }

    const uint8_t instruction = chunk->code[offset];
    switch (instruction) {
    case OP_ARRAY: return longOperandInstruction("OP_ARRAY", chunk, offset);
    case OP_CONSTANT: return constantInstruction("OP_CONSTANT", chunk, offset);
    case OP_NIL: return simpleInstruction("OP_NIL", offset);
    case OP_TRUE: return simpleInstruction("OP_TRUE", offset);
    case OP_FALSE: return simpleInstruction("OP_FALSE", offset);
    case OP_POP: return simpleInstruction("OP_POP", offset);
    case OP_DUP: return simpleInstruction("OP_DUP", offset);
    case OP_GET_LOCAL: return byteInstruction("OP_GET_LOCAL", chunk, offset);
    case OP_SET_LOCAL: return byteInstruction("OP_SET_LOCAL", chunk, offset);
    case OP_GET_GLOBAL: return constantInstruction("OP_GET_GLOBAL", chunk, offset);
    case OP_DEFINE_GLOBAL: return constantInstruction("OP_DEFINE_GLOBAL", chunk, offset);
    case OP_SET_GLOBAL: return constantInstruction("OP_SET_GLOBAL", chunk, offset);
    case OP_GET_UPVALUE: return byteInstruction("OP_GET_UPVALUE", chunk, offset);
    case OP_SET_UPVALUE: return byteInstruction("OP_SET_UPVALUE", chunk, offset);
    case OP_STATIC_FIELD: return constantInstruction("OP_STATIC_FIELD", chunk, offset);
    case OP_GET_PROPERTY: return constantInstruction("OP_GET_PROPERTY", chunk, offset);
    case OP_SET_PROPERTY: return constantInstruction("OP_SET_PROPERTY", chunk, offset);
    case OP_GET_INDEX: return simpleInstruction("OP_GET_INDEX", offset);
    case OP_SET_INDEX: return simpleInstruction("OP_SET_INDEX", offset);
    case OP_GET_SUPER: return constantInstruction("OP_GET_SUPER", chunk, offset);
    case OP_EQUAL: return simpleInstruction("OP_EQUAL", offset);
    case OP_GREATER: return simpleInstruction("OP_GREATER", offset);
    case OP_LESS: return simpleInstruction("OP_LESS", offset);
    case OP_ADD: return simpleInstruction("OP_ADD", offset);
    case OP_SUBTRACT: return simpleInstruction("OP_SUBTRACT", offset);
    case OP_MULTIPLY: return simpleInstruction("OP_MULTIPLY", offset);
    case OP_MODULUS: return simpleInstruction("OP_MODULUS", offset);
    case OP_EXPONENT: return simpleInstruction("OP_EXPONENT", offset);
    case OP_DIVIDE: return simpleInstruction("OP_DIVIDE", offset);
    case OP_NOT: return simpleInstruction("OP_NOT", offset);
    case OP_NEGATE: return simpleInstruction("OP_NEGATE", offset);
    case OP_PRINT: return simpleInstruction("OP_PRINT", offset);
    case OP_JUMP: return jumpInstruction("OP_JUMP", 1, chunk, offset);
    case OP_JUMP_IF_FALSE: return jumpInstruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
    case OP_LOOP: return jumpInstruction("OP_LOOP", -1, chunk, offset);
    case OP_CALL: return byteInstruction("OP_CALL", chunk, offset);
    case OP_INVOKE: return invokeInstruction("OP_INVOKE", chunk, offset);
    case OP_CLOSURE: return closureInstruction("OP_CLOSURE", chunk, offset);
    case OP_CLOSE_UPVALUE: return simpleInstruction("OP_CLOSE_UPVALUE", offset);
    case OP_RETURN: return simpleInstruction("OP_RETURN", offset);
    case OP_CLASS: return constantInstruction("OP_CLASS", chunk, offset);
    case OP_INHERIT: return simpleInstruction("OP_INHERIT", offset);
    case OP_METHOD: return constantInstruction("OP_METHOD", chunk, offset);
    case OP_STATIC_METHOD: return constantInstruction("OP_STATIC_METHOD", chunk, offset);
    case OP_THROW: return simpleInstruction("OP_THROW", offset);
    case OP_PUSH_EXCEPTION_HANDLER:
        return exceptionHandlerInstruction("OP_PUSH_EXCEPTION_HANDLER", chunk, offset);
    case OP_POP_EXCEPTION_HANDLER: return simpleInstruction("OP_POP_EXCEPTION_HANDLER", offset);
    case OP_PROPAGATE_EXCEPTION: return simpleInstruction("OP_PROPAGATE_EXCEPTION", offset);
    default: printf("Unknown opcode %d\n", instruction);
        return offset + 1;
    }
}
