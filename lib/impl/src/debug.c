#include <stdio.h>

#include <impl/object.h>
#include <impl/debug.h>

static const char* opcodeToString(OpCode opcode) {
    static const char* text[] = {
#define ENUM_OPCODE_DEF(name) #name,
    OPCODE_ENUM_LIST
#undef ENUM_OBJTYPE_DEF
    };
    if (opcode < 0 || opcode >= OP_LAST) {
        return "unknown opcode";
    }
    return text[opcode];
}

void disassembleChunk(FILE* file, Chunk* chunk, const char* name) {
    fprintf(file, "== %s ==\n", name);

    for (int offset = 0; offset < chunk->count;) {
        offset = disassembleInstruction(file, chunk, offset);
    }
}

static uint8_t read_byte(const Chunk* chunk, const int offset) {
    return chunk->code[offset];
}

static uint16_t read_short(const Chunk* chunk, const int offset) {
    return (chunk->code[offset] << 8) | chunk->code[offset + 1];
}

static int constantInstruction(FILE* file, const char* name, const Chunk* chunk, const int offset) {
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
    const char* desc = opcodeToString(instruction);
    switch (instruction) {
    case OP_ARRAY: return longOperandInstruction(file, desc, chunk, offset);
    case OP_CONSTANT: return constantInstruction(file, desc, chunk, offset);
    case OP_CONSTANT_ZERO: return simpleInstruction(file, desc, offset);
    case OP_CONSTANT_ONE: return simpleInstruction(file, desc, offset);
    case OP_CONSTANT_TWO: return simpleInstruction(file, desc, offset);
    case OP_NIL: return simpleInstruction(file, desc, offset);
    case OP_TRUE: return simpleInstruction(file, desc, offset);
    case OP_FALSE: return simpleInstruction(file, desc, offset);
    case OP_POP: return simpleInstruction(file, desc, offset);
    case OP_DUP: return simpleInstruction(file, desc, offset);
    case OP_GET_LOCAL: return byteInstruction(file, desc, chunk, offset);
    case OP_SET_LOCAL: return byteInstruction(file, desc, chunk, offset);
    case OP_GET_GLOBAL: return constantInstruction(file, desc, chunk, offset);
    case OP_DEFINE_GLOBAL: return constantInstruction(file, desc, chunk, offset);
    case OP_SET_GLOBAL: return constantInstruction(file, desc, chunk, offset);
    case OP_GET_UPVALUE: return byteInstruction(file, desc, chunk, offset);
    case OP_SET_UPVALUE: return byteInstruction(file, desc, chunk, offset);
    case OP_STATIC_FIELD: return constantInstruction(file, desc, chunk, offset);
    case OP_GET_PROPERTY: return constantInstruction(file, desc, chunk, offset);
    case OP_SET_PROPERTY: return constantInstruction(file, desc, chunk, offset);
    case OP_GET_INDEX: return simpleInstruction(file, desc, offset);
    case OP_SET_INDEX: return simpleInstruction(file, desc, offset);
    case OP_GET_SUPER: return constantInstruction(file, desc, chunk, offset);
    case OP_EQUAL: return simpleInstruction(file, desc, offset);
    case OP_GREATER: return simpleInstruction(file, desc, offset);
    case OP_LESS: return simpleInstruction(file, desc, offset);
    case OP_ADD: return simpleInstruction(file, desc, offset);
    case OP_SUBTRACT: return simpleInstruction(file, desc, offset);
    case OP_MULTIPLY: return simpleInstruction(file, desc, offset);
    case OP_MODULUS: return simpleInstruction(file, desc, offset);
    case OP_EXPONENT: return simpleInstruction(file, desc, offset);
    case OP_DIVIDE: return simpleInstruction(file, desc, offset);
    case OP_NOT: return simpleInstruction(file, desc, offset);
    case OP_NEGATE: return simpleInstruction(file, desc, offset);
    case OP_PRINT: return simpleInstruction(file, desc, offset);
    case OP_JUMP: return jumpInstruction(file, desc, 1, chunk, offset);
    case OP_JUMP_IF_FALSE: return jumpInstruction(file, desc, 1, chunk, offset);
    case OP_LOOP: return jumpInstruction(file, desc, -1, chunk, offset);
    case OP_CALL: return byteInstruction(file, desc, chunk, offset);
    case OP_INVOKE: return invokeInstruction(file, desc, chunk, offset);
    case OP_SUPER_INVOKE: return invokeInstruction(file, desc, chunk, offset); 
    case OP_CLOSURE: return closureInstruction(file, desc, chunk, offset);
    case OP_CLOSE_UPVALUE: return simpleInstruction(file, desc, offset);
    case OP_RETURN: return simpleInstruction(file, desc, offset);
    case OP_CLASS: return constantInstruction(file, desc, chunk, offset);
    case OP_INHERIT: return simpleInstruction(file, desc, offset);
    case OP_METHOD: return constantInstruction(file, desc, chunk, offset);
    case OP_STATIC_METHOD: return constantInstruction(file, desc, chunk, offset);
    case OP_THROW: return simpleInstruction(file, desc, offset);
    case OP_PUSH_EXCEPTION_HANDLER:
        return exceptionHandlerInstruction(file, desc, chunk, offset);
    case OP_POP_EXCEPTION_HANDLER: return simpleInstruction(file, desc, offset);
    case OP_PROPAGATE_EXCEPTION: return simpleInstruction(file, desc, offset);
    default: fprintf(file, "Unknown opcode %d\n", instruction);
        return offset + 1;
    }
}
