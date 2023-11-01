//
// Created by djumi on 10/29/2023.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include "../h/common.h"
#include "../h/value.h"
#include "../h/compiler.h"
#include "../h/debug.h"
#include "../h/memory.h"
#include "../h/object.h"

VM vm;

static bool clockNative(int argCount, Value *args) {
    args[-1] = NUMBER_VAL((double) clock() / CLOCKS_PER_SEC);
    return true;
}

static bool exitNative(int argCount, Value *args) {
    if (!IS_NUMBER(args[0])) {
        const char *message = "Exit code must be a number.";
        args[-1] = OBJ_VAL((Obj *) copyString(message, strlen(message)));
        return false;
    }
    int exitCode = (int) AS_NUMBER(args[0]);
    exit(exitCode);
    args[-1] = NIL_VAL;
    return true;
}

static void resetStack() {
    vm.stackTop = vm.stack;
    vm.frameCount = 0;
    vm.openUpvalues = NULL;
}

static inline ObjFunction *getFrameFunction(CallFrame *frame) {
    if (frame->function->type == OBJ_FUNCTION) {
        return (ObjFunction *) frame->function;
    } else {
        return ((ObjClosure *) frame->function)->function;
    }
}

void runtimeError(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    for (int i = vm.frameCount - 1; i >= 0; i--) {
        CallFrame *frame = &vm.frames[i];
        ObjFunction *function = getFrameFunction(frame);
        size_t instruction = frame->ip - function->chunk.code - 1;
        int line = getLine(&function->chunk, (int) instruction);
        fprintf(stderr, "[line %d] in ", line);
        if (function->name == NULL) {
            fprintf(stderr, "script\n");
        } else {
            fprintf(stderr, "%s()\n", function->name->chars);
        }
    }

    resetStack();
}

static void defineNative(const char *name, int arity, NativeFn function) {
    push(OBJ_VAL((Obj *) copyString(name, (int) strlen(name))));
    push(OBJ_VAL((Obj *) newNative(function, arity)));
    tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
    pop();
    pop();
}

void initVM() {
    resetStack();
    initTable(&vm.globals);
    initTable(&vm.strings);
    vm.objects = NULL;

    vm.bytesAllocated = 0;
    vm.nextGC = 512;

    vm.grayCount = 0;
    vm.grayCapacity = 0;
    vm.grayStack = NULL;

    defineNative("clock", 0, clockNative);
    defineNative("exit", 1, exitNative);
}

void freeVM() {
    freeTable(&vm.globals);
    freeTable(&vm.strings);
    freeObjects();
#ifdef DEBUG_LOG_GC
    printf("%td bytes still allocated by the VM.\n", vm.bytesAllocated);
#endif
}

void push(Value value) {
    *vm.stackTop = value;
    vm.stackTop++;
}

Value pop() {
    vm.stackTop--;
    return *vm.stackTop;
}

static Value peek(int distance) {
    return vm.stackTop[-1 - distance];
}

bool callNonNative(Obj *callee, ObjFunction *function, int argCount) {
    if (argCount != function->arity) {
        runtimeError("Expected %d arguments but got %d",
                     function->arity, argCount);
        return false;
    }

    if (vm.frameCount == FRAMES_MAX) {
        runtimeError("Stack overflow.");
        return false;
    }

    CallFrame *frame = &vm.frames[vm.frameCount++];
    frame->function = (Obj *) callee;
    frame->ip = function->chunk.code;
    frame->slots = vm.stackTop - argCount - 1;
    return true;
}

static bool callValue(Value callee, int argCount) {
    /* TODO: This can further be optimized by giving every object a call attr
        Callable struct field 'caller' can be moved into object and implemented
        for all of the Obj types. For types that are not callable, runtime error
        can be reported and /false/ fan be returned directly
     */
    if (IS_CALLABLE(callee)) return CALL_CALLABLE_V(callee, argCount);
    runtimeError("Can only call functions and classes.");
    return false;
}

static ObjUpvalue *captureUpvalue(Value *local) {
    ObjUpvalue *prevUpvalue = NULL;
    ObjUpvalue *upvalue = vm.openUpvalues;
    while (upvalue != NULL && upvalue->location > local) {
        prevUpvalue = upvalue;
        upvalue = upvalue->next;
    }

    if (upvalue != NULL && upvalue->location == local) {
        return upvalue;
    }

    ObjUpvalue *createdUpvalue = newUpvalue(local);
    createdUpvalue->next = upvalue;

    if (prevUpvalue == NULL) {
        vm.openUpvalues = createdUpvalue;
    } else {
        prevUpvalue->next = createdUpvalue;
    }

    return createdUpvalue;
}

static void closeUpvalues(Value *last) {
    while (vm.openUpvalues != NULL && vm.openUpvalues->location >= last) {
        ObjUpvalue *upvalue = vm.openUpvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        vm.openUpvalues = upvalue->next;
    }
}

static bool isFalsy(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate() {
    ObjString *b = AS_STRING(peek(0));
    ObjString *a = AS_STRING(peek(1));

    int length = a->length + b->length;
    char *chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString *result = takeString(chars, length);
    pop();
    pop();
    push(OBJ_VAL((Obj *) result));
}

static InterpretResult run() {
    CallFrame *frame = &vm.frames[vm.frameCount - 1];
    register uint8_t *ip = frame->ip;

#define READ_BYTE() (*ip++)
#define READ_SHORT() (ip+=2, (uint16_t)((ip[-2] << 8) | ip[-1]))
#define READ_CONSTANT() (getFrameFunction(frame)->chunk.constants.values[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define BINARY_OP(valueType, op) \
    do {              \
        if(!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))){ \
            frame->ip = ip;                     \
            runtimeError("Operands must be numbers");   \
            return INTERPRETER_RUNTIME_ERROR; \
        }\
        double b = AS_NUMBER(pop()); \
        double a = AS_NUMBER(pop()); \
        push(valueType(a op b)); \
    } while(false)


    while (true) {
#ifdef DEBUG_TRACE_EXECUTION
        printf("\t\t");
        for (Value *slot = vm.stack; slot < vm.stackTop; slot++) {
            printf("[ ");
            printValue(*slot);
            printf(" ]");
        }
        printf("\n");
        disassembleInstruction(
                &getFrameFunction(frame)->chunk,
                (int) (ip - getFrameFunction(frame)->chunk.code));
#endif
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
        case OP_CONSTANT: {
            push(READ_CONSTANT());
            break;
        }
        case OP_NIL: push(NIL_VAL);
            break;
        case OP_TRUE:push(BOOL_VAL(true));
            break;
        case OP_FALSE: push(BOOL_VAL(false));
            break;
        case OP_POP: pop();
            break;
        case OP_DUP: push(peek(0));
            break;
        case OP_GET_LOCAL: {
            uint8_t slot = READ_BYTE();
            push(frame->slots[slot]);
            break;
        }
        case OP_SET_LOCAL: {
            uint8_t slot = READ_BYTE();
            frame->slots[slot] = peek(0);
            break;
        }
        case OP_GET_GLOBAL: {
            ObjString *name = READ_STRING();
            Value value;
            if (!tableGet(&vm.globals, name, &value)) {
                frame->ip = ip;
                runtimeError("Undefined variable '%s'.", name->chars);
                return INTERPRETER_RUNTIME_ERROR;
            }
            push(value);
            break;
        }
        case OP_DEFINE_GLOBAL: {
            ObjString *name = READ_STRING();
            tableSet(&vm.globals, name, peek(0));
            pop();
            break;
        }
        case OP_SET_GLOBAL: {
            ObjString *name = READ_STRING();
            if (tableSet(&vm.globals, name, peek(0))) {
                tableDelete(&vm.globals, name);
                frame->ip = ip;
                runtimeError("Undefined variable '%s'.", name->chars);
                return INTERPRETER_RUNTIME_ERROR;
            }
            break;
        }
        case OP_GET_UPVALUE: {
            uint8_t slot = READ_BYTE();
            push(*((ObjClosure *) frame->function)->upvalues[slot]->location);
            break;
        }
        case OP_SET_UPVALUE: {
            uint8_t slot = READ_BYTE();
            *((ObjClosure *) frame->function)->upvalues[slot]->location = peek(0);
            break;
        }
        case OP_EQUAL: {
            Value b = pop();
            Value a = pop();
            push(BOOL_VAL(valuesEqual(a, b)));
            break;
        }
        case OP_GREATER: BINARY_OP(BOOL_VAL, >);
            break;
        case OP_LESS: BINARY_OP(BOOL_VAL, <);
            break;
        case OP_ADD: {
            if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                concatenate();
            } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                double b = AS_NUMBER(pop());
                double a = AS_NUMBER(pop());
                push(NUMBER_VAL(a + b));
            } else {
                frame->ip = ip;
                runtimeError("Operands must be two numbers or two strings.");
                return INTERPRETER_RUNTIME_ERROR;
            }
            break;
        }
        case OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *);
            break;
        case OP_DIVIDE: BINARY_OP(NUMBER_VAL, /);
            break;
        case OP_NOT: push(BOOL_VAL(isFalsy(pop())));
            break;
        case OP_NEGATE:
            if (!IS_NUMBER(peek(0))) {
                frame->ip = ip;
                runtimeError("Operand must be a number.");
                return INTERPRETER_RUNTIME_ERROR;
            }
            push(NUMBER_VAL(-AS_NUMBER(pop())));
            break;
        case OP_JUMP: {
            uint16_t offset = READ_SHORT();
            ip += offset;
            break;
        }
        case OP_JUMP_IF_FALSE: {
            uint16_t offset = READ_SHORT();
            if (isFalsy(peek(0))) ip += offset;
            break;
        }
        case OP_LOOP: {
            uint16_t offset = READ_SHORT();
            ip -= offset;
            break;
        }
        case OP_PRINT: printValue(pop());
            printf("\n");
            break;
        case OP_CALL: {
            int argCount = READ_BYTE();
            frame->ip = ip;
            if (!callValue(peek(argCount), argCount)) {
                return INTERPRETER_RUNTIME_ERROR;
            }
            frame = &vm.frames[vm.frameCount - 1];
            ip = frame->ip;
            break;
        }
        case OP_CLOSURE: {
            ObjFunction *function = AS_FUNCTION(READ_CONSTANT());
            ObjClosure *closure = newClosure(function);
            push(OBJ_VAL((Obj *) closure));
            for (int i = 0; i < closure->upvalueCount; i++) {
                uint8_t isLocal = READ_BYTE();
                uint8_t index = READ_BYTE();
                if (isLocal) {
                    closure->upvalues[i] =
                            captureUpvalue(frame->slots + index);
                } else {
                    closure->upvalues[i] =
                            ((ObjClosure *) frame->function)->upvalues[index];
                }
            }
            break;
        }
        case OP_CLOSE_UPVALUE: {
            closeUpvalues(vm.stackTop - 1);
            pop();
            break;
        }
        case OP_RETURN: {
            Value result = pop();
            closeUpvalues(frame->slots);
            vm.frameCount--;
            if (vm.frameCount == 0) {
                pop();
                return INTERPRETER_OK;
            }

            vm.stackTop = frame->slots;
            push(result);
            frame = &vm.frames[vm.frameCount - 1];
            ip = frame->ip;
            break;
        }
        }
    }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}

InterpretResult interpret(const char *source) {
    ObjFunction *function = compile(source);
    if (function == NULL) return INTERPRETER_COMPILE_ERROR;
    push(OBJ_VAL((Obj *) function));
//    callFunction(function, 0);
//    ((Callable *) function)->caller((Callable *) function, 0);
    CALL_CALLABLE_O(function, 0);

    return run();
}
