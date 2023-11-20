//
// Created by djumi on 10/29/2023.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include "../h/common.h"
#include "../h/value.h"
#include "../h/compiler.h"
#include "../h/debug.h"
#include "../h/memory.h"
#include "../h/object.h"
#include "../h/native.h"

VM vm;

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

static ObjClass *nativeClass(const char *name) {
    push(OBJ_VAL((Obj *) copyString(name, (int) strlen(name))));
    ObjClass *klass = newClass(AS_STRING(vm.stack[0]));
    push(OBJ_VAL((Obj *) klass));
    tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
    pop();
    pop();
    return klass;
}

static bool initExceptionNative(int argCount, Value *args) {
    if (argCount > 1) {
        args[-1] = NATIVE_ERROR("Exit takes either 0 arguments or one a string.");
        return false;
    }
    ObjInstance *exception = AS_INSTANCE(args[-1]);
    const char *message = "message";
    int msg_len = (int) strlen(message);
    ObjString *messageString = tableFindOrAddString(
            &vm.strings, message,
            msg_len, hashString(message, msg_len));
    if (argCount == 0) {
        const char *defaultText = "Exception occurred.";
        int len = (int) strlen(defaultText);
        ObjString *messageText = tableFindOrAddString(
                &vm.strings, defaultText,
                len, hashString(defaultText, len));
        tableSet(&exception->fields, messageString, OBJ_VAL(messageText));
    } else {
        if (!IS_STRING(args[0])) {
            args[-1] = NATIVE_ERROR("Expected a string as an argument");
            return false;
        }
        tableSet(&exception->fields, messageString, OBJ_VAL(args[0]));
    }
    args[-1] = OBJ_VAL((Obj *) exception);
    return true;
}

static void addNativeMethod(ObjClass *klass, const char *name, NativeFn method, int arity) {
    push(OBJ_VAL((Obj *) copyString(name, (int) strlen(name))));
    push(OBJ_VAL((Obj *) newNative(method, arity)));
    tableSet(&klass->methods, AS_STRING(vm.stack[0]), vm.stack[1]);
    if (AS_STRING(vm.stack[0]) == vm.initString) klass->initializer = vm.stack[1];
    pop();
    pop();
}

void initVM() {
    resetStack();
    vm.objects = NULL;

    vm.grayCount = 0;
    vm.grayCapacity = 0;
    vm.grayStack = NULL;
    vm.bytesAllocated = 0;
    vm.nextGC = 1024 * 1024;

    initTable(&vm.globals);
    initTable(&vm.strings);

    // Make sure initString is not null
    // because of GC
    vm.initString = NULL;
    vm.initString = copyString("init", 4);

    // Define native methods
    for (int i = 0; nativeMethods[i].name != NULL; i++) {
        NativeMethodDef *def = &nativeMethods[i];
        defineNative(def->name, def->arity, def->function);
    }

    ObjClass *exception = nativeClass("Exception");
    addNativeMethod(exception, "init", initExceptionNative, -1);
}

void freeVM() {
    freeTable(&vm.globals);
    freeTable(&vm.strings);
    vm.initString = NULL;
    freeObjects();
    free(vm.grayStack);
#ifdef DEBUG_LOG_GC
    printf("%td bytes still allocated by the VM.\n", vm.bytesAllocated);
#endif
}

void push(Value value) {
    *vm.stackTop++ = value;
}

Value pop() {
    return *--vm.stackTop;
}

static Value peek(int distance) {
    return vm.stackTop[-1 - distance];
}


static bool callFunctionLike(Obj *callee, ObjFunction *function, int argCount) {
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

bool callClosure(Obj *callable, int argCount) {
    return callFunctionLike(callable, ((ObjClosure *) callable)->function, argCount);
}

bool callFunction(Obj *callable, int argCount) {
    return callFunctionLike(callable, (ObjFunction *) callable, argCount);
}

bool callClass(Obj *callable, int argCount) {
    ObjClass *klass = (ObjClass *) callable;
    vm.stackTop[-argCount - 1] = OBJ_VAL((Obj *) newInstance(klass));
    if (!IS_NIL(klass->initializer)) {
        return CALL_OBJ(klass->initializer, argCount);
    } else if (argCount != 0) {
        runtimeError("Expected 0 arguments but got %d.", argCount);
        return false;
    }
    return true;
}

bool callBoundMethod(Obj *callable, int argCount) {
    ObjBoundMethod *bound = (ObjBoundMethod *) callable;
    vm.stackTop[-argCount - 1] = bound->receiver;
    return bound->method->vtp->call(bound->method, argCount);
}

bool callNative(Obj *callable, int argCount) {
    ObjNative *native = (ObjNative *) callable;
    if (native->arity != -1 && argCount != native->arity) {
        runtimeError("Expected %d arguments but got %d", native->arity, argCount);
        return false;
    }

    if (native->function(argCount, vm.stackTop - argCount)) {
        vm.stackTop -= argCount;
        return true;
    } else {
        runtimeError(AS_STRING(vm.stackTop[-argCount - 1])->chars);
        return false;
    }
}

static bool callValue(Value callee, int argCount) {
    if (IS_OBJ(callee)) return CALL_OBJ(callee, argCount);
    runtimeError("Can only call functions and classes.");
    return false;
}

static bool invokeFromImpl(Table *methods, ObjString *name, int argCount) {
    Value method;
    if (tableGet(methods, name, &method)) {
        return CALL_OBJ(method, argCount);
    }
    runtimeError("Undefined property '%s'.", name->chars);
    return false;
}

static bool invoke(ObjString *name, int argCount) {
    Value receiver = peek(argCount);

    if (!IS_INSTANCE(receiver) && !IS_CLASS(receiver)) {
        runtimeError("Only classes and instances have methods");
        return false;
    }

    Obj *object = AS_OBJ(receiver);

    Value value;
    Table *fields = IS_CLASS(receiver)
                    ? &((ObjClass *) object)->fields
                    : &((ObjInstance *) object)->fields;
    if (tableGet(fields, name, &value)) {
        if (IS_INSTANCE(value)) {
            vm.stackTop[-argCount - 1] = value;
        }
        return callValue(value, argCount);
    }

    if (IS_CLASS(receiver)) {
        return invokeFromImpl(
                &((ObjClass *) object)->staticMethods,
                name, argCount);
    } else {
        return invokeFromImpl(
                &((ObjInstance *) object)->klass->methods,
                name, argCount);
    }
}

static bool bindMethod(ObjClass *klass, ObjString *name) {
    Value method;
    if (!tableGet(&klass->methods, name, &method)) {
        runtimeError("Undefined property '%s'.", name->chars);
        return false;
    }

    ObjBoundMethod *bound = newBoundMethod(peek(0), AS_OBJ(method));
    pop();
    push(OBJ_VAL((Obj *) bound));
    return true;
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

static void closeUpvalues(const Value *last) {
    while (vm.openUpvalues != NULL && vm.openUpvalues->location >= last) {
        ObjUpvalue *upvalue = vm.openUpvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        vm.openUpvalues = upvalue->next;
    }
}

static void defineMethod(ObjString *name) {
    Value method = peek(0);
    ObjClass *klass = AS_CLASS(peek(1));
    tableSet(&klass->methods, name, method);
    if (name == vm.initString) klass->initializer = method;
    pop();
}

static void defineStaticMethod(ObjString *name) {
    Value method = peek(0);
    ObjClass *klass = AS_CLASS(peek(1));
    tableSet(&klass->staticMethods, name, method);
    pop();
}

static bool isFalsy(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static ObjString *concatenateImpl(char const *buffer1, int length1, char const *buffer2, int length2) {
    int length = length1 + length2;
    char *chars = ALLOCATE(char, (size_t) length + 1);
    snprintf(chars, length + 1, "%s%s", buffer1, buffer2);
    chars[length] = '\0';
    return takeString(chars, length);
}

static void concatenate() {
    ObjString *b = AS_STRING(peek(0));
    ObjString *a = AS_STRING(peek(1));
    ObjString *result = concatenateImpl(
            a->chars, a->length,
            b->chars, b->length
    );
    pop();
    pop();
    push(OBJ_VAL((Obj *) result));
}

static int primitiveStringLength(Value value) {
    if (IS_NIL(value)) return 3;
    else if (IS_BOOL(value)) return AS_BOOL(value) ? 4 : 5;
    else if (IS_NUMBER(value)) return snprintf(NULL, 0, "%g", AS_NUMBER(value));
    return 0; // Unreachable
}

static void writePrimitiveToBuffer(char *buffer, Value value, int length) {
    if (IS_NIL(value)) memcpy(buffer, "nil", length + 1);
    else if (IS_BOOL(value)) memcpy(buffer, AS_BOOL(value) ? "true" : "false", length + 1);
    else if (IS_NUMBER(value)) snprintf(buffer, length + 1, "%g", AS_NUMBER(value));
}

static void concatenateStringWithPrimitive() {
    ObjString *b = AS_STRING(peek(0));
    Value a = peek(1);

    int primitiveStrLen = primitiveStringLength(a);
    char primitiveStr[primitiveStrLen + 1];
    writePrimitiveToBuffer(primitiveStr, a, primitiveStrLen);

    ObjString *result = concatenateImpl(
            primitiveStr, primitiveStrLen,
            b->chars, b->length);
    pop();
    pop();
    push(OBJ_VAL((Obj *) result));
}

static void concatenatePrimitiveWithString() {
    Value b = peek(0);
    ObjString *a = AS_STRING(peek(1));

    int primitiveStrLen = primitiveStringLength(b);
    char primitiveStr[primitiveStrLen + 1];
    writePrimitiveToBuffer(primitiveStr, b, primitiveStrLen);

    ObjString *result = concatenateImpl(
            a->chars, a->length,
            primitiveStr, primitiveStrLen);
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
    do {                         \
        if(!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))){ \
            frame->ip = ip;                     \
            runtimeError("Operands must be numbers");   \
            return INTERPRET_RUNTIME_ERROR; \
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
            if (IS_STRING(*slot)) printf("\"");
            printValue(*slot);
            if (IS_STRING(*slot)) printf("\"");
            printf(" ]");
        }
        printf("\n");
        disassembleInstruction(
                &getFrameFunction(frame)->chunk,
                (int) (ip - getFrameFunction(frame)->chunk.code));
#endif
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
        case OP_ARRAY: {
            ObjArray *array = newArray();
            size_t size = READ_SHORT();
            Value *elements = vm.stackTop - size;
            push(OBJ_VAL(array));
            for (size_t i = 0; i < size; i++) {
                writeValueArray(&array->array, elements[i]);
            }
            vm.stackTop = elements;
            push(OBJ_VAL(array));
            break;
        }
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
                return INTERPRET_RUNTIME_ERROR;
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
                return INTERPRET_RUNTIME_ERROR;
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
        case OP_STATIC_FIELD: {
            ObjString *field = READ_STRING();
            Value value = peek(0);
            ObjClass *klass = AS_CLASS(peek(1)); // Class
            tableSet(&klass->fields, field, value);
            pop();
            break;
        }
        case OP_GET_PROPERTY: {
            if (!IS_INSTANCE(peek(0)) && !IS_CLASS(peek(0))) {
                frame->ip = ip;
                runtimeError("Only instances and classes have properties.");
                return INTERPRET_RUNTIME_ERROR;
            }
            if (IS_INSTANCE(peek(0))) {
                ObjInstance *instance = AS_INSTANCE(peek(0));
                ObjString *name = READ_STRING();

                Value value;
                if (tableGet(&instance->fields, name, &value)) {
                    pop(); // Instance
                    push(value);
                    break;
                }
                if (!bindMethod(instance->klass, name)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
            } else {
                ObjClass *klass = AS_CLASS(peek(0));
                ObjString *name = READ_STRING();

                Value value;
                if (tableGet(&klass->staticMethods, name, &value) ||
                    tableGet(&klass->fields, name, &value)) {
                    pop(); // Class
                    push(value);
                    break;
                }
                frame->ip = ip;
                runtimeError("No static member '%s' on class '%s'.",
                             name->chars, klass->name->chars);
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        }
        case OP_SET_PROPERTY: {
            if (!IS_INSTANCE(peek(1)) && !IS_CLASS(peek(1))) {
                frame->ip = ip;
                runtimeError("Only instances have fields.");
                return INTERPRET_RUNTIME_ERROR;
            }
            Value receiver = peek(1);
            Table *fields = IS_INSTANCE(receiver)
                            ? &AS_INSTANCE(receiver)->fields
                            : &AS_CLASS(receiver)->fields;
            tableSet(fields, READ_STRING(), peek(0));
            Value value = pop();
            pop();
            push(value);

            break;
        }
        case OP_GET_INDEX: {
            if (!IS_NUMBER(peek(0))) {
                frame->ip = ip;
                runtimeError("Index must be a number.");
                return INTERPRET_RUNTIME_ERROR;
            }
            if (!IS_OBJ_ARRAY(peek(1))) {
                frame->ip = ip;
                runtimeError("Only arrays are indexable.");
                return INTERPRET_RUNTIME_ERROR;
            }
            ptrdiff_t index = (ptrdiff_t) AS_NUMBER(peek(0));
            ObjArray *array = AS_ARRAY(peek(1));
            if (index < 0 || index >= array->array.count) {
                frame->ip = ip;
                runtimeError("Array index out of bounds. Index = %td", index);
                return INTERPRET_RUNTIME_ERROR;
            }
            pop();
            pop();
            push(array->array.values[index]);
            break;
        }
        case OP_SET_INDEX: {
            if (!IS_NUMBER(peek(1))) {
                frame->ip = ip;
                runtimeError("Index must be a number.");
                return INTERPRET_RUNTIME_ERROR;
            }
            if (!IS_OBJ_ARRAY(peek(2))) {
                frame->ip = ip;
                runtimeError("Only arrays are indexable.");
                return INTERPRET_RUNTIME_ERROR;
            }
            ptrdiff_t index = (ptrdiff_t) AS_NUMBER(peek(1));
            ObjArray *array = AS_ARRAY(peek(2));
            if (index < 0 || index >= array->array.count) {
                frame->ip = ip;
                runtimeError("Array index out of bounds. Index = %td", index);
                return INTERPRET_RUNTIME_ERROR;
            }
            Value value = pop();
            array->array.values[index] = value;
            pop();
            pop();
            push(value);
            break;
        }
        case OP_GET_SUPER: {
            ObjString *name = READ_STRING();
            ObjClass *superclass = AS_CLASS(pop());

            if (!bindMethod(superclass, name)) {
                return INTERPRET_RUNTIME_ERROR;
            }
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
            /*
             * TODO: instead of coercing primitives into strings,
             *  which only allows for them to bi concatenated with strings,
             *  call a version of "toString" for a value, before concatenating
             *  it with a string
             * */
            if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                concatenate();
            } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                double b = AS_NUMBER(pop());
                double a = AS_NUMBER(pop());
                push(NUMBER_VAL(a + b));
            } else if (IS_STRING(peek(0)) && !IS_OBJ(peek(1))) {
                concatenateStringWithPrimitive();
            } else if (!IS_OBJ(peek(0)) && IS_STRING(peek(1))) {
                concatenatePrimitiveWithString();
            } else {
                frame->ip = ip;
                runtimeError("Operands must be primitives or strings.");
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        }
        case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -);
            break;
        case OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *);
            break;
        case OP_DIVIDE: BINARY_OP(NUMBER_VAL, /);
            break;
        case OP_MODULUS: {
            if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                double b = AS_NUMBER(pop());
                double a = AS_NUMBER(pop());
                push(NUMBER_VAL(fmod(a, b)));
            } else {
                frame->ip = ip;
                runtimeError("Operands must be two numbers.");
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        }
        case OP_NOT: push(BOOL_VAL(isFalsy(pop())));
            break;
        case OP_NEGATE:
            if (!IS_NUMBER(peek(0))) {
                frame->ip = ip;
                runtimeError("Operand must be a number.");
                return INTERPRET_RUNTIME_ERROR;
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
                return INTERPRET_RUNTIME_ERROR;
            }
            frame = &vm.frames[vm.frameCount - 1];
            ip = frame->ip;
            break;
        }
        case OP_INVOKE: {
            ObjString *method = READ_STRING();
            int argCount = READ_BYTE();
            frame->ip = ip;
            if (!invoke(method, argCount)) {
                return INTERPRET_RUNTIME_ERROR;
            }
            frame = &vm.frames[vm.frameCount - 1];
            ip = frame->ip;
            break;
        }
        case OP_SUPER_INVOKE: {
            ObjString *method = READ_STRING();
            int argCount = READ_BYTE();
            ObjClass *superclass = AS_CLASS(pop());
            if (!invokeFromImpl(&superclass->methods, method, argCount)) {
                return INTERPRET_RUNTIME_ERROR;
            }
            frame->ip = ip;
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
                return INTERPRET_OK;
            }
            vm.stackTop = frame->slots;
            push(result);
            frame = &vm.frames[vm.frameCount - 1];
            ip = frame->ip;
            break;
        }
        case OP_CLASS: {
            push(OBJ_VAL((Obj *) newClass(READ_STRING())));
            break;
        }
        case OP_INHERIT: {
            Value superclass = peek(1);
            if (!IS_CLASS(superclass)) {
                frame->ip = ip;
                runtimeError("Superclass must be a class.");
                return INTERPRET_RUNTIME_ERROR;
            }
            ObjClass *subclass = AS_CLASS(peek(0));
            tableAddAll(&AS_CLASS(superclass)->methods,
                        &subclass->methods);
            pop(); // Subclass
            break;
        }
        case OP_METHOD: {
            ObjString *name = READ_STRING();
            defineMethod(name);
            break;
        }
        case OP_STATIC_METHOD: {
            ObjString *name = READ_STRING();
            defineStaticMethod(name);
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
    if (function == NULL) return INTERPRET_COMPILE_ERROR;
    push(OBJ_VAL((Obj *) function));
    callFunction((Obj *) function, 0);

    return run();
}

InterpretResult interpretCompiled(ObjFunction *function) {
    if (function == NULL) return INTERPRET_RUNTIME_ERROR;
    push(OBJ_VAL((Obj *) function));
    callFunction((Obj *) function, 0);

    return run();
}