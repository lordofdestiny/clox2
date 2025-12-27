#include <stdio.h>

#include <sys/types.h>

#include "object.h"
#include "debug.h"

// extern int lines;
// extern const char** file_lines;

void disassembleChunk(FILE* file, Chunk* chunk, const char* name) {
    fprintf(file, "== %s ==\n", name);

    for (int offset = 0; offset < chunk->count;) {
        int line = getLine(chunk, offset);
        // if(offset == 0 || offset > 0 && line != getLine(chunk, offset - 1) && line < lines) {
        //     fprintf(file, "%4d.\t%3s\n",line, file_lines[line - 1]);
        // }

        offset = disassembleInstruction(file, chunk, offset);
    }
}

static uint8_t read_byte(const Chunk* chunk, const int offset) {
    return chunk->code[offset];
}

static uint16_t read_short(const Chunk* chunk, const int offset) {
    return (chunk->code[offset] << 8) | chunk->code[offset + 1];
}

static int constantInstruction(FILE* file, char* name, const Chunk* chunk, const int offset) {
    const uint8_t constant = read_byte(chunk, offset + 1);
    fprintf(file, "%-29s %4d '", name, constant);
    printValue(file, chunk->constants.values[constant]);
    fprintf(file, "'\n");
    return offset + 2;
}

static int longOperandInstruction(FILE* file, const char* name, const Chunk* chunk, const int offset) {
    const uint16_t constant = read_short(chunk, offset + 1);
    fprintf(file, "%-29s %4d\n", name, constant);
    return offset + 3;
}

static int invokeInstruction(FILE* file, const char* name, const Chunk* chunk, const int offset) {
    const uint8_t constant = read_byte(chunk, offset + 1);
    const uint8_t argCount = read_byte(chunk, offset + 2);
    fprintf(file, "%-29s (%d args) %4d '", name, argCount, constant);
    printValue(file, chunk->constants.values[constant]);
    fprintf(file, "'\n");
    return offset + 3;
}

static int simpleInstruction(FILE* file, const char* name, const int offset) {
    fprintf(file, "%s\n", name);
    return offset + 1;
}

static int byteInstruction(FILE* file, const char* name, const Chunk* chunk, const int offset) {
    const uint8_t slot = read_byte(chunk, offset + 1);
    fprintf(file, "%-29s %4d\n", name, slot);
    return offset + 2;
}

static int jumpInstruction(FILE* file, const char* name, const int sign, const Chunk* chunk, const int offset) {
    const uint16_t jump = read_short(chunk, offset + 1);
    fprintf(file, "%-29s %4d -> %d\n", name, offset, offset + 3 + sign * jump);
    return offset + 3;
}

static int closureInstruction(FILE* file, const char* name, const Chunk* chunk, int offset) {
    offset++;
    const uint8_t constant = read_byte(chunk, offset++);
    fprintf(file, "%-29s %4d ", name, constant);
    printValue(file, chunk->constants.values[constant]);
    fprintf(file, "\n");

    const ObjFunction* function = AS_FUNCTION(chunk->constants.values[constant]);
    for (int j = 0; j < function->upvalueCount; j++) {
        const int isLocal = read_byte(chunk, offset++);
        const int index = read_byte(chunk, offset++);
        fprintf(
            file,
            "%04d\t|\t\t\t%s %d\n",
            offset - 2, isLocal ? "local" : "upvalue", index);
    }
    return offset;
}

static int exceptionHandlerInstruction(FILE* file, const char* name, const Chunk* chunk, const int offset) {
    const uint8_t type = chunk->code[offset + 1];

    const uint16_t handlerAddress = read_short(chunk, offset + 2);
    const uint16_t finallyAddress = read_short(chunk, offset + 4);

    if (finallyAddress != 0xFFFF) {
        fprintf(file, "%-29s %4d -> %d, %d\n", name, type, handlerAddress, finallyAddress);
    } else {
        fprintf(file, "%-29s %4d -> %d\n", name, type, handlerAddress);
    }
    return offset + 6;
}

int disassembleInstruction(FILE* file, Chunk* chunk, const int offset) {
    fprintf(file, "%04d ", offset);
    const int line = getLine(chunk, offset);
    if (offset > 0 && line == getLine(chunk, offset - 1)) {
        fprintf(file, "\t| ");
    } else {
       fprintf(file, "%4d ", line);
    }

    const uint8_t instruction = chunk->code[offset];
    switch (instruction) {
    case OP_ARRAY: return longOperandInstruction(file, "OP_ARRAY", chunk, offset);
    case OP_CONSTANT: return constantInstruction(file, "OP_CONSTANT", chunk, offset);
    case OP_CONSTANT_MINUS_ONE: return simpleInstruction(file, "OP_CONSTANT_MINUS_ONE", offset);
    case OP_CONSTANT_ZERO: return simpleInstruction(file, "OP_CONSTANT_ZERO", offset);
    case OP_CONSTANT_ONE: return simpleInstruction(file, "OP_CONSTANT_ONE", offset);
    case OP_CONSTANT_TWO: return simpleInstruction(file, "OP_CONSTANT_TWO", offset);
    case OP_NIL: return simpleInstruction(file,"OP_NIL", offset);
    case OP_TRUE: return simpleInstruction(file,"OP_TRUE", offset);
    case OP_FALSE: return simpleInstruction(file,"OP_FALSE", offset);
    case OP_POP: return simpleInstruction(file,"OP_POP", offset);
    case OP_DUP: return simpleInstruction(file,"OP_DUP", offset);
    case OP_GET_LOCAL: return byteInstruction(file,"OP_GET_LOCAL", chunk, offset);
    case OP_SET_LOCAL: return byteInstruction(file,"OP_SET_LOCAL", chunk, offset);
    case OP_GET_GLOBAL: return constantInstruction(file,"OP_GET_GLOBAL", chunk, offset);
    case OP_DEFINE_GLOBAL: return constantInstruction(file,"OP_DEFINE_GLOBAL", chunk, offset);
    case OP_SET_GLOBAL: return constantInstruction(file,"OP_SET_GLOBAL", chunk, offset);
    case OP_GET_UPVALUE: return byteInstruction(file,"OP_GET_UPVALUE", chunk, offset);
    case OP_SET_UPVALUE: return byteInstruction(file,"OP_SET_UPVALUE", chunk, offset);
    case OP_STATIC_FIELD: return constantInstruction(file,"OP_STATIC_FIELD", chunk, offset);
    case OP_GET_PROPERTY: return constantInstruction(file,"OP_GET_PROPERTY", chunk, offset);
    case OP_SET_PROPERTY: return constantInstruction(file,"OP_SET_PROPERTY", chunk, offset);
    case OP_GET_INDEX: return simpleInstruction(file,"OP_GET_INDEX", offset);
    case OP_SET_INDEX: return simpleInstruction(file,"OP_SET_INDEX", offset);
    case OP_GET_SUPER: return constantInstruction(file,"OP_GET_SUPER", chunk, offset);
    case OP_EQUAL: return simpleInstruction(file,"OP_EQUAL", offset);
    case OP_GREATER: return simpleInstruction(file,"OP_GREATER", offset);
    case OP_LESS: return simpleInstruction(file,"OP_LESS", offset);
    case OP_ADD: return simpleInstruction(file,"OP_ADD", offset);
    case OP_SUBTRACT: return simpleInstruction(file,"OP_SUBTRACT", offset);
    case OP_MULTIPLY: return simpleInstruction(file,"OP_MULTIPLY", offset);
    case OP_MODULUS: return simpleInstruction(file,"OP_MODULUS", offset);
    case OP_EXPONENT: return simpleInstruction(file,"OP_EXPONENT", offset);
    case OP_DIVIDE: return simpleInstruction(file,"OP_DIVIDE", offset);
    case OP_NOT: return simpleInstruction(file,"OP_NOT", offset);
    case OP_NEGATE: return simpleInstruction(file,"OP_NEGATE", offset);
    case OP_PRINT: return simpleInstruction(file,"OP_PRINT", offset);
    case OP_JUMP: return jumpInstruction(file,"OP_JUMP", 1, chunk, offset);
    case OP_JUMP_IF_FALSE: return jumpInstruction(file,"OP_JUMP_IF_FALSE", 1, chunk, offset);
    case OP_LOOP: return jumpInstruction(file,"OP_LOOP", -1, chunk, offset);
    case OP_CALL: return byteInstruction(file,"OP_CALL", chunk, offset);
    case OP_INVOKE: return invokeInstruction(file, "OP_INVOKE", chunk, offset);
    case OP_CLOSURE: return closureInstruction(file, "OP_CLOSURE", chunk, offset);
    case OP_CLOSE_UPVALUE: return simpleInstruction(file,"OP_CLOSE_UPVALUE", offset);
    case OP_RETURN: return simpleInstruction(file,"OP_RETURN", offset);
    case OP_CLASS: return constantInstruction(file,"OP_CLASS", chunk, offset);
    case OP_INHERIT: return simpleInstruction(file,"OP_INHERIT", offset);
    case OP_METHOD: return constantInstruction(file,"OP_METHOD", chunk, offset);
    case OP_STATIC_METHOD: return constantInstruction(file,"OP_STATIC_METHOD", chunk, offset);
    case OP_THROW: return simpleInstruction(file,"OP_THROW", offset);
    case OP_PUSH_EXCEPTION_HANDLER:
        return exceptionHandlerInstruction(file, "OP_PUSH_EXCEPTION_HANDLER", chunk, offset);
    case OP_POP_EXCEPTION_HANDLER: return simpleInstruction(file,"OP_POP_EXCEPTION_HANDLER", offset);
    case OP_PROPAGATE_EXCEPTION: return simpleInstruction(file,"OP_PROPAGATE_EXCEPTION", offset);
    default: fprintf(file, "Unknown opcode %d\n", instruction);
        return offset + 1;
    }
}
