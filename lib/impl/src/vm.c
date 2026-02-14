#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <setjmp.h>

#include <dlfcn.h>

#include <clox/value.h>
#include <clox/vm.h>

#include <impl/compiler.h>
#include <impl/memory.h>
#include <impl/native.h>
#include <impl/object.h>
#include <impl/vm.h>


#if defined(DEBUG_PRINT_CODE) || defined(DEBUG_TRACE_EXECUTION)

#include <impl/debug.h>

#endif

#define FAILED_LIB_LOAD 50
#define FAILED_REF_STACK_FULL 55
#define FAILED_STACK_UNDERFLOW 60
#define FAILED_STACK_OVERFLOW 70

VM vm;
NativeLibraryState nativeState;

[[noreturn]]
__attribute__((noreturn))
void terminate(int code) {
    if (!vm.exit_state_ready){
        fprintf(stderr, "FATAL: terminate() called from VM before jump state was set\n");
        abort();
    }
    vm.exit_code = code;
    longjmp(vm.exit_state, 1);
    __builtin_unreachable();
}

static void resetStack() {
    vm.stackTop = vm.stack;
    vm.frameCount = 0;
    vm.openUpvalues = NULL;
    for (int i = 0; i < vm.frameCount; i++) {
        vm.frames[i].handlerCount = 0;
    }
}

static inline ObjFunction* getFrameFunction(const CallFrame* frame) {
    if (frame->function->type == OBJ_FUNCTION) {
        return (ObjFunction*) frame->function;
    }

    return ((ObjClosure*) frame->function)->function;
}

void runtimeError(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");

    for (int i = vm.frameCount - 1; i >= 0; i--) {
        const CallFrame* frame = &vm.frames[i];
        ObjFunction* function = getFrameFunction(frame);
        const size_t instruction = frame->ip - function->chunk.code - 1;
        const int line = getLine(&function->chunk, (int) instruction);

        const char* name = function->name ? function->name->chars : "script";
        fprintf(stderr, "[line %d] in %s\n", line, name);
    }

    resetStack();
}

static ObjClass* getGlobalClass(const char* name) {
    Value value;
    if (tableGet(&vm.globals, copyString(name, (int) strlen(name)), &value)) {
        if (!IS_CLASS(value)) return NULL;
        return (ObjClass*) AS_OBJ(value);
    }
    return NULL;
}

static bool defineNative(const char* name, const int arity, const NativeFn function) {
    PUSH_OBJ(copyString(name, (int) strlen(name)));
    PUSH_OBJ(newNative(name, function, arity));

    if (tableGet(&vm.globals, AS_STRING(vm.stack[0]), NULL)) {
        pop();
        pop(); 
        runtimeError("Function '%s' already registered!", name);
        terminate(FAILED_LIB_LOAD);
    }

    tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
    pop();
    pop();
    
    return true;
}

static ObjClass* nativeClass(const char* name) {
    PUSH_OBJ(copyString(name, (int) strlen(name)));
    ObjClass* klass = newClass(AS_STRING(vm.stack[0]));
    PUSH_OBJ(klass);
    tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
    pop();
    pop();
    return klass;
}

static void addNativeMethod(
    ObjClass* klass, const char* name, const NativeFn method, const int arity
) {
    PUSH_OBJ(copyString(name, (int) strlen(name)));
    PUSH_OBJ(newNative(name, method, arity));
    tableSet(&klass->methods, AS_STRING(vm.stack[0]), vm.stack[1]);
    if (AS_STRING(vm.stack[0]) == vm.initString) klass->initializer = vm.stack[1];
    pop();
    pop();
}

#define LOG_LIB_LOADED 0

typedef size_t (*GetCountFn)(void); 

typedef size_t (*RegisterFunctionsFn)(DefineNativeFunctionFn);

static int loadNativeLib(const char* libPath) {
    void* handle = dlopen(libPath, RTLD_NOW);
    if (handle == NULL) goto load_error_msg;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    const char* const libraryName = dlsym(handle,"CLOX_MODULE_NAME");
    if (libraryName == NULL) goto load_error_msg;

    LibraryEventFn onLoadFn = (LibraryEventFn) dlsym(handle, "onLoad");
    if (onLoadFn == NULL) goto load_error_msg;
    
    LibraryEventFn onUnload = (LibraryEventFn) dlsym(handle, "onUnload");
    if (onUnload == NULL) goto load_error_msg;
    
    RegisterFunctionsFn registerFunctions = (RegisterFunctionsFn) dlsym(handle, "registerFunctions");
    if (registerFunctions == NULL) goto load_error_msg;
#pragma GCC diagnostic pop

    [[maybe_unused]] size_t count = registerFunctions(defineNative);
    
    size_t newSize = (nativeState.nativeLibCount + 1) * sizeof(*nativeState.nativeLibHandles);
    NativeLibrary* handleArr = realloc(nativeState.nativeLibHandles, newSize);
    if (handleArr == NULL) {
        fprintf(stderr, "dlib error: out of memory while loading '%s' (%s)\n", libraryName, libPath);
        goto load_error;
    }

    nativeState.nativeLibHandles = handleArr;
    NativeLibrary lib = {
        .name = libraryName,
        .handle = handle,
        .onLoad = onLoadFn,
        .onUnload = onUnload
    };

    handleArr[nativeState.nativeLibCount++] = lib;

    onLoadFn();
    
#if LOG_LIB_LOADED
    fprintf(stdout, ""
        "Library loaded:\n"
        "  * Name: %s\n"
        "  * Path: %s\n"
        "  * Fucntions: %zu\n"
        "  * Classes: Not supported\n"
        "\n",
        lib.name, libPath, count
    );
#endif

    return 0;

load_error_msg:
    fprintf(stderr, "dlib error: %s\n", dlerror());
load_error:
    if (handle != NULL) dlclose(handle);
    terminate(FAILED_LIB_LOAD);
}

static void initNative() {
    nativeState.nativeLibCount = 0;
    nativeState.nativeLibHandles = NULL;
    
    nativeState.nativeRcNext = 0;

    nativeState.nativeArgsCap = 0;
    nativeState.nativeArgs = NULL;

    static const char* libs[] = {
        "libcloxreflect.so", 
        "libcloxtime.so", 
        "libcloxsystem.so",
        "libcloxmath.so"
    };
    for (size_t i = 0; i < sizeof(libs)/sizeof(libs[0]); i++) {
        loadNativeLib(libs[i]);
    }

    ObjClass* exception = nativeClass("Exception");
    addNativeMethod(exception, "init", initExceptionNative, -1);

    ObjClass* number = nativeClass("Number");
    addNativeMethod(number, "init", initNumberNative, -1);
    addNativeMethod(number, "toPrecision", toPrecisionNative, 1);

    ObjClass* boolean = nativeClass("Boolean");
    addNativeMethod(boolean, "init", initBooleanNative, -1);

    ObjClass* string = nativeClass("String");
    addNativeMethod(string, "init", initStringNative, -1);

    ObjClass* array = nativeClass("Array");
    addNativeMethod(array, "init", initArrayNative, -1);
    addNativeMethod(array, "append", appendArrayNative, 1);
    addNativeMethod(array, "pop", popArrayNative, 0);
}

void initVM() {
    resetStack();
    vm.objects = NULL;
    vm.exit_code = 0;
    vm.exit_state_ready = false;

    if (setjmp(vm.exit_state) != 0) {
        fprintf(stderr, "FATAL: VM initialization failed\n");
        exit(255);
        return;
    }

    vm.exit_state_ready = true;
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

    initNative();
    vm.exit_state_ready = false;
}

void freeVM() {
    for (size_t i = 0; i < nativeState.nativeLibCount; i++) {
        nativeState.nativeLibHandles[i].onUnload();
        dlclose(nativeState.nativeLibHandles[i].handle);
    }
    free(nativeState.nativeLibHandles);
    free(nativeState.nativeArgs);
    
    freeTable(&vm.globals);
    freeTable(&vm.strings);
    vm.initString = NULL;
    freeObjects();
    free(vm.grayStack);
#ifdef DEBUG_LOG_GC
    printf("%td bytes still allocated by the VM.\n", vm.bytesAllocated);
#endif
}

int vmExitCode() {
    return vm.exit_code;
}

int referenceScope() {
    return nativeState.nativeRcNext;
}

void pushReference(Value val) {
    if (nativeState.nativeRcNext == MAX_NATIVE_RC) {
        runtimeError("Native function reference stack overflow [cap=%d]", MAX_NATIVE_RC);
        terminate(FAILED_REF_STACK_FULL);
    }
    nativeState.nativeRc[nativeState.nativeRcNext++] = val;
}

Value popReference() {
    if (nativeState.nativeRcNext <= 0) {
        runtimeError("Native function reference stack underflow");
        terminate(FAILED_REF_STACK_FULL);
    }
    return nativeState.nativeRc[--nativeState.nativeRcNext];
}

void resetReferences(int scope) {
    nativeState.nativeRcNext = scope;
}


void push(const Value value) {
    if (vm.stackTop >= vm.stack + STACK_MAX) {
        runtimeError( "Stack overflow");
        terminate(FAILED_STACK_OVERFLOW);
    }
    *vm.stackTop++ = value;
}

Value pop() {
    if (vm.stackTop <= vm.stack) {
        runtimeError( "Stack underflow");
        terminate(FAILED_STACK_UNDERFLOW);
    }
    return *--vm.stackTop;
}

static Value peek(const int distance) {
    return vm.stackTop[-1 - distance];
}

static void unpackPrimitive(const int distance) {
    const Value current = peek(distance);
    Value unpacked;
    if (IS_INSTANCE(current) &&
        !(IS_INSTANCE(unpacked = AS_INSTANCE(current)->this_))) {
        vm.stackTop[-1 - distance] = unpacked;
    }
}

static bool promote(const int distance, ObjClass* klass) {
    const Value value = peek(distance);
    PUSH_OBJ(newPrimitive(value, klass)); // This
    push(value); // Value being promoted passed as an argument to the ctor
    if (CALL_OBJ(klass->initializer, 1)) { // call the class ctor
        const Value promoted = pop(); // Pop result of ctor call - promoted value
        vm.stackTop[-1 - distance] = promoted;

        return true;
    }
    return false;
}

static bool tryPromote(const int distance) {
    const Value value = peek(distance);
    if (!IS_OBJ(value)) {
        if (IS_NIL(value)) return false;
        if (IS_BOOL(value)) return promote(distance, getGlobalClass("Boolean"));

        return promote(distance, getGlobalClass("Number")); // NUMBER
    }

    if (IS_ARRAY(value)) return promote(distance, getGlobalClass("Array"));
    if (IS_STRING(value)) return promote(distance, getGlobalClass("String"));

    return IS_CLASS(value) || IS_INSTANCE(value);
}

static Value getStackTrace(void) {
#define MAX_LINE_LENGTH 512
    const int maxStackTraceLength = vm.frameCount * MAX_LINE_LENGTH;
    char* stackTrace = ALLOCATE(char, maxStackTraceLength);
    uint16_t index = 0;
    for (int i = vm.frameCount - 1; i >= 0; i--) {
        const CallFrame* frame = &vm.frames[i];
        ObjFunction* function = getFrameFunction(frame);
        const ptrdiff_t instruction = frame->ip - function->chunk.code - 1;
        const uint32_t lineno = getLine(&function->chunk, (int) instruction);
        index += snprintf(
            &stackTrace[index], MAX_LINE_LENGTH,
            "[line %d] in %s()\n", lineno,
            function->name == NULL ? "script" : function->name->chars);
    }
    stackTrace = GROW_ARRAY(char, stackTrace, maxStackTraceLength, index + 1);
    return OBJ_VAL((Obj*) takeString(stackTrace, index));
#undef MAX_LINE_LENGTH
}

static bool instanceof(const ObjInstance* instance, const Value klass) {
    return IS_CLASS(klass) && instance->klass == AS_CLASS(klass);
}

static void closeUpvalues(const Value* last);

static bool propagateException(void) {
#define PLACEHOLDER_ADDRESS 0xffff
    const Value value = peek(0);
    if (!IS_INSTANCE(value)) {
        fprintf(stderr, "Unhandled ");
        printValue(stderr, value);
        fprintf(stderr, "\n");
        return false;
    }
    ObjInstance* exception = AS_INSTANCE(value);

    while (vm.frameCount > 0) {
        CallFrame* frame = &vm.frames[vm.frameCount - 1];
        for (int numHandlers = frame->handlerCount; numHandlers > 0; numHandlers--) {
            const ExceptionHandler handler = frame->handlerStack[numHandlers - 1];
            if (instanceof(exception, handler.klass)) {
                frame->handlerCount = numHandlers;
                frame->ip = &getFrameFunction(frame)->chunk.code[handler.handlerAddress];
                closeUpvalues(frame->slots);
                return true;
            }

            if (handler.finallyAddress != PLACEHOLDER_ADDRESS) {
                push(TRUE_VAL);
                frame->handlerCount = numHandlers;
                frame->ip = &getFrameFunction(frame)->chunk.code[handler.finallyAddress];
                return true;
            }
        }
        vm.frameCount--;
    }
    fprintf(stderr, "Unhandled %s", exception->klass->name->chars);
    Value exceptionClass, message;
    if (tableGet(&vm.globals, copyString("Exception", 9), &exceptionClass) &&
        exception->klass == AS_CLASS(exceptionClass) &&
        tableGet(&exception->fields, copyString("message", 7), &message) &&
        IS_STRING(message)) {
        fprintf(stderr, ": \"%s\"", AS_STRING(message)->chars);
    }
    fprintf(stderr, "\n");
    Value stacktrace;
    if (tableGet(&exception->fields, copyString("stackTrace", 10), &stacktrace)) {
        fprintf(stderr, "%s", AS_CSTRING(stacktrace));
        fflush(stderr);
    }
    return false;
#undef PLACEHOLDER_ADDRESS
}

static void pushExceptionHandler(
    const Value type, const uint16_t handlerAddress, const uint16_t finallyAddress
) {
    CallFrame* frame = &vm.frames[vm.frameCount - 1];
    if (frame->handlerCount == MAX_HANDLER_FRAMES) {
        runtimeError("Too many nexted exception handlers in one function.");
        return;
    }
    frame->handlerStack[frame->handlerCount++] = (ExceptionHandler){
        .klass = type,
        .handlerAddress = handlerAddress,
        .finallyAddress = finallyAddress,
    };
}

static bool callFunctionLike(Obj* callee, const ObjFunction* function, const int argCount) {
    if (argCount != function->arity) {
        runtimeError(
            "Expected %d arguments but got %d",
            function->arity, argCount);
        return false;
    }

    if (vm.frameCount == FRAMES_MAX) {
        runtimeError("Stack overflow.");
        return false;
    }

    CallFrame* frame = &vm.frames[vm.frameCount++];
    frame->function = (Obj*) callee;
    frame->ip = function->chunk.code;
    frame->slots = vm.stackTop - argCount - 1;
    frame->handlerCount = 0;
    return true;
}

bool callClosure(Obj* callable, const int argCount) {
    return callFunctionLike(callable, ((ObjClosure*) callable)->function, argCount);
}

bool callFunction(Obj* callable, const int argCount) {
    return callFunctionLike(callable, (ObjFunction*) callable, argCount);
}

bool callClass(Obj* callable, const int argCount) {
    ObjClass* klass = (ObjClass*) callable;
    vm.stackTop[-argCount - 1] = OBJ_VAL((Obj *) newInstance(klass));
    if (!IS_NIL(klass->initializer)) {
        return CALL_OBJ(klass->initializer, argCount);
    }

    if (argCount != 0) {
        runtimeError("Expected 0 arguments but got %d.", argCount);
        return false;
    }

    return true;
}

bool callBoundMethod(Obj* callable, const int argCount) {
    const ObjBoundMethod* bound = (ObjBoundMethod*) callable;
    vm.stackTop[-argCount - 1] = bound->receiver;
    return bound->method->vtp->call(bound->method, argCount);
}

bool callNative(Obj* callable, const int argCount) {
    const ObjNative* native = (ObjNative*) callable;
    if (native->arity != -1 && argCount != native->arity) {
        runtimeError("Expected %d arguments but got %d", native->arity, argCount);
        return false;
    }

    if (nativeState.nativeArgsCap < argCount) {
        size_t newCap = argCount + 1;
        nativeState.nativeArgs = GROW_ARRAY(Value, nativeState.nativeArgs, nativeState.nativeArgsCap, newCap);
        nativeState.nativeArgsCap = newCap;
    }

    Value* const stack = vm.stackTop - argCount -1;
    memcpy(nativeState.nativeArgs, stack, sizeof(Value) * (argCount + 1));
    
    if (native->function(argCount, nativeState.nativeArgs, nativeState.nativeArgs + 1)) {
        vm.stackTop -= argCount;
        vm.stackTop[-1] = *nativeState.nativeArgs;
        return true;
    }
    

    runtimeError("Native function failed");
    return false;
}

static bool callValue(const Value callee, const int argCount) {
    if (IS_OBJ(callee)) return CALL_OBJ(callee, argCount);

    runtimeError("Can only call functions and classes.");
    return false;
}

static bool invokeFromImpl(Table* methods, ObjString* name, const int argCount) {
    Value method;
    if (tableGet(methods, name, &method)) {
        return CALL_OBJ(method, argCount);
    }

    runtimeError("Undefined property '%s'.", name->chars);
    return false;
}

static bool invoke(ObjString* name, const int argCount) {
    const Value receiver = peek(argCount);

    if (!IS_INSTANCE(receiver) && !IS_CLASS(receiver)) {
        runtimeError("Only classes and instances have methods");
        return false;
    }

    Obj* object = AS_OBJ(receiver);

    Value value;
    Table* fields = IS_CLASS(receiver)
                        ? &((ObjClass*) object)->fields
                        : &((ObjInstance*) object)->fields;

    if (tableGet(fields, name, &value)) {
        if (IS_INSTANCE(value)) {
            vm.stackTop[-argCount - 1] = value;
        }
        return callValue(value, argCount);
    }

    if (IS_CLASS(receiver)) {
        return invokeFromImpl(&((ObjClass*) object)->staticMethods, name, argCount);
    }

    return invokeFromImpl(&((ObjInstance*) object)->klass->methods, name, argCount);
}

static bool bindMethod(ObjClass* klass, ObjString* name) {
    Value method;
    if (!tableGet(&klass->methods, name, &method)) {
        runtimeError("Undefined property '%s'.", name->chars);
        return false;
    }

    ObjBoundMethod* bound = newBoundMethod(peek(0), AS_OBJ(method));
    pop();
    PUSH_OBJ(bound);
    return true;
}

static ObjUpvalue* getUpvalue(const CallFrame* frame, const int slot) {
    return ((ObjClosure*) frame->function)->upvalues[slot];
}

static ObjUpvalue* captureUpvalue(Value* local) {
    ObjUpvalue* prevUpvalue = NULL;
    ObjUpvalue* upvalue = vm.openUpvalues;
    while (upvalue != NULL && upvalue->location > local) {
        prevUpvalue = upvalue;
        upvalue = upvalue->next;
    }

    if (upvalue != NULL && upvalue->location == local) {
        return upvalue;
    }

    ObjUpvalue* createdUpvalue = newUpvalue(local);
    createdUpvalue->next = upvalue;

    if (prevUpvalue == NULL) {
        vm.openUpvalues = createdUpvalue;
    } else {
        prevUpvalue->next = createdUpvalue;
    }

    return createdUpvalue;
}

static void closeUpvalues(const Value* last) {
    while (vm.openUpvalues != NULL &&
           vm.openUpvalues->location >= last) {
        ObjUpvalue* upvalue = vm.openUpvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        vm.openUpvalues = upvalue->next;
    }
}

static void defineMethod(ObjString* name) {
    const Value method = peek(0);
    ObjClass* klass = AS_CLASS(peek(1));
    tableSet(&klass->methods, name, method);
    if (name == vm.initString) klass->initializer = method;
    pop();
}

static void defineStaticMethod(ObjString* name) {
    const Value method = peek(0);
    ObjClass* klass = AS_CLASS(peek(1));
    tableSet(&klass->staticMethods, name, method);
    pop();
}

static bool isFalsy(const Value value) {
    return IS_NIL(value)
           || (IS_BOOL(value) && !AS_BOOL(value))
           || (IS_INSTANCE(value)
               && IS_BOOL(AS_INSTANCE(value)->this_)
               && !AS_BOOL(AS_INSTANCE(value)->this_));
}

static ObjString* concatenateImpl(
    char const* buffer1, const int length1,
    char const* buffer2, const int length2
) {
    const int length = length1 + length2;
    char* chars = ALLOCATE(char, (size_t) length + 1);
    snprintf(chars, length + 1, "%s%s", buffer1, buffer2);
    chars[length] = '\0';
    return takeString(chars, length);
}

static void concatenate() {
    const ObjString* b = AS_STRING(peek(0));
    const ObjString* a = AS_STRING(peek(1));
    ObjString* result = concatenateImpl(
        a->chars, a->length,
        b->chars, b->length
    );
    pop();
    pop();
    PUSH_OBJ(result);
}

static int primitiveStringLength(const Value value) {
    if (IS_NIL(value)) return 3;
    if (IS_BOOL(value)) return AS_BOOL(value) ? 4 : 5;
    if (IS_NUMBER(value)) return snprintf(NULL, 0, "%g", AS_NUMBER(value));
    return 0; // Unreachable
}

static void writePrimitiveToBuffer(char* buffer, const Value value, const int length) {
    if (IS_NIL(value)) memcpy(buffer, "nil", length + 1);
    else if (IS_BOOL(value)) memcpy(buffer, AS_BOOL(value) ? "true" : "false", length + 1);
    else if (IS_NUMBER(value)) snprintf(buffer, length + 1, "%g", AS_NUMBER(value));
}

static void concatenateStringWithPrimitive() {
    const ObjString* b = AS_STRING(peek(0));
    const Value a = peek(1);

    const int primitiveStrLen = primitiveStringLength(a);
    char primitiveStr[primitiveStrLen + 1];
    writePrimitiveToBuffer(primitiveStr, a, primitiveStrLen);

    ObjString* result = concatenateImpl(
        primitiveStr, primitiveStrLen,
        b->chars, b->length);
    pop();
    pop();
    PUSH_OBJ(result);
}

static void concatenatePrimitiveWithString() {
    const Value b = peek(0);
    const ObjString* a = AS_STRING(peek(1));

    const int primitiveStrLen = primitiveStringLength(b);
    char primitiveStr[primitiveStrLen + 1];
    writePrimitiveToBuffer(primitiveStr, b, primitiveStrLen);

    ObjString* result = concatenateImpl(
        a->chars, a->length,
        primitiveStr, primitiveStrLen);
    pop();
    pop();
    PUSH_OBJ(result);
}

static InterpretResult run() {
    CallFrame* frame = &vm.frames[vm.frameCount - 1];
    register uint8_t* ip = frame->ip;

#define READ_BYTE() (*ip++)
#define READ_SHORT() (ip+=2, (uint16_t)((ip[-2] << 8) | ip[-1]))
#define READ_CONSTANT() (getFrameFunction(frame)->chunk.constants.values[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define BINARY_OP(valueType, op) \
    do {                         \
        unpackPrimitive(0); \
        unpackPrimitive(1); \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))){ \
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
            printValue(stdout, *slot);
            if (IS_STRING(*slot)) printf("\"");
            printf(" ]");
        }
        printf("\n");
        disassembleInstruction(
            stdout,
                &getFrameFunction(frame)->chunk,
                (int) (ip - getFrameFunction(frame)->chunk.code));
#endif

        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
        case OP_ARRAY: {
            ObjArray* array = newArray();
            size_t size = READ_SHORT();
            Value* elements = vm.stackTop - size;
            PUSH_OBJ(array);
            for (size_t i = 0; i < size; i++) {
                writeValueArray(&array->array, elements[i]);
            }
            vm.stackTop = elements;
            PUSH_OBJ(array);
            break;
        }
        case OP_CONSTANT: push(READ_CONSTANT());
            break;
        case OP_CONSTANT_ZERO:
        case OP_CONSTANT_ONE:
        case OP_CONSTANT_TWO:
            push(NUMBER_VAL(instruction - OP_CONSTANT_ZERO));
            break;
        case OP_NIL: push(NIL_VAL);
            break;
        case OP_TRUE: push(BOOL_VAL(true));
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
            ObjString* name = READ_STRING();
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
            ObjString* name = READ_STRING();
            tableSet(&vm.globals, name, peek(0));
            pop();
            break;
        }
        case OP_SET_GLOBAL: {
            ObjString* name = READ_STRING();
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
            push(*getUpvalue(frame, slot)->location);
            break;
        }
        case OP_SET_UPVALUE: {
            uint8_t slot = READ_BYTE();
            *getUpvalue(frame, slot)->location = peek(0);
            break;
        }
        case OP_STATIC_FIELD: {
            ObjString* field = READ_STRING();
            Value value = peek(0);
            ObjClass* klass = AS_CLASS(peek(1));
            tableSet(&klass->fields, field, value);
            pop();
            break;
        }
        case OP_GET_PROPERTY: {
            tryPromote(0);
            if (!IS_INSTANCE(peek(0)) &&
                !IS_CLASS(peek(0))) {
                frame->ip = ip;
                runtimeError("Only instances and classes have properties.");
                return INTERPRET_RUNTIME_ERROR;
            }
            if (IS_INSTANCE(peek(0))) {
                ObjInstance* instance = AS_INSTANCE(peek(0));
                ObjString* name = READ_STRING();

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
                ObjClass* klass = AS_CLASS(peek(0));
                ObjString* name = READ_STRING();

                Value value;
                if (tableGet(&klass->staticMethods, name, &value) ||
                    tableGet(&klass->fields, name, &value)) {
                    pop(); // Class
                    push(value);
                    break;
                }
                frame->ip = ip;
                runtimeError(
                    "No static member '%s' on class '%s'.",
                    name->chars, klass->name->chars);
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        }
        case OP_SET_PROPERTY: {
            tryPromote(1);
            if (!IS_INSTANCE(peek(1)) && !IS_CLASS(peek(1))) {
                frame->ip = ip;
                runtimeError("Only instances have fields.");
                return INTERPRET_RUNTIME_ERROR;
            }
            Value receiver = peek(1);
            Table* fields = IS_INSTANCE(receiver)
                                ? &AS_INSTANCE(receiver)->fields
                                : &AS_CLASS(receiver)->fields;
            tableSet(fields, READ_STRING(), peek(0));
            Value value = pop();
            pop();
            push(value);

            break;
        }
        case OP_GET_INDEX: {
            unpackPrimitive(0);
            unpackPrimitive(1);
            if (!IS_NUMBER(peek(0))) {
                frame->ip = ip;
                runtimeError("Index must be a number.");
                return INTERPRET_RUNTIME_ERROR;
            }
            if (!IS_ARRAY(peek(1))) {
                frame->ip = ip;
                runtimeError("Only arrays are indexable.");
                return INTERPRET_RUNTIME_ERROR;
            }
            ptrdiff_t index = (ptrdiff_t) AS_NUMBER(peek(0));
            ObjArray* array = AS_ARRAY(peek(1));
            if (index < 0 || index >= array->array.count) {
                frame->ip = ip;
                runtimeError(
                    "Array index out of bounds. Length = %d, Index = %td", array->array.count,
                    index);
                return INTERPRET_RUNTIME_ERROR;
            }
            pop();
            pop();
            push(array->array.values[index]);
            break;
        }
        case OP_SET_INDEX: {
            unpackPrimitive(1);
            unpackPrimitive(2);
            if (!IS_NUMBER(peek(1))) {
                frame->ip = ip;
                runtimeError("Index must be a number.");
                return INTERPRET_RUNTIME_ERROR;
            }
            if (!IS_ARRAY(peek(2))) {
                frame->ip = ip;
                runtimeError("Only arrays are indexable.");
                return INTERPRET_RUNTIME_ERROR;
            }
            ptrdiff_t index = (ptrdiff_t) AS_NUMBER(peek(1));
            ObjArray* array = AS_ARRAY(peek(2));
            if (index < 0 || index >= array->array.count) {
                frame->ip = ip;
                runtimeError(
                    "Array index out of bounds. Length = %d, Index = %td", array->array.count,
                    index);
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
            ObjString* name = READ_STRING();
            ObjClass* superclass = AS_CLASS(pop());

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
            /**
             *  TODO instead of coercing primitives into strings,
             *  which only allows for them to be concatenated with strings,
             *  call a version of "toString" for a value, before concatenating
             *  it with a string
             **/
            unpackPrimitive(0);
            unpackPrimitive(1);
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
        case OP_EXPONENT: {
            unpackPrimitive(0);
            unpackPrimitive(1);
            if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                double b = AS_NUMBER(pop());
                double a = AS_NUMBER(pop());
                push(NUMBER_VAL(pow(a, b)));
            } else {
                frame->ip = ip;
                runtimeError("Operands must be two numbers.");
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        }
        case OP_DIVIDE: BINARY_OP(NUMBER_VAL, /);
            break;
        case OP_MODULUS: {
            unpackPrimitive(0);
            unpackPrimitive(1);
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
        case OP_NEGATE: {
            unpackPrimitive(0);
            if (!IS_NUMBER(peek(0))) {
                frame->ip = ip;
                runtimeError("Operand must be a number.");
                return INTERPRET_RUNTIME_ERROR;
            }
            if (IS_NUMBER(peek(0))) {
                push(NUMBER_VAL(-AS_NUMBER(pop())));
            } else {
                ObjInstance* instance = AS_INSTANCE(peek(0));
                instance->this_ = NUMBER_VAL(-AS_NUMBER(instance->this_));
            }
            break;
        }
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
        case OP_PRINT: printValue(stdout, pop());
            fprintf(stdout, "\n");
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
            ObjString* method = READ_STRING();
            int argCount = READ_BYTE();
            tryPromote(argCount);
            frame->ip = ip;
            if (!invoke(method, argCount)) {
                return INTERPRET_RUNTIME_ERROR;
            }
            frame = &vm.frames[vm.frameCount - 1];
            ip = frame->ip;
            break;
        }
        case OP_SUPER_INVOKE: {
            ObjString* method = READ_STRING();
            int argCount = READ_BYTE();
            ObjClass* superclass = AS_CLASS(pop());
            frame->ip = ip;
            if (!invokeFromImpl(&superclass->methods, method, argCount)) {
                return INTERPRET_RUNTIME_ERROR;
            }
            frame = &vm.frames[vm.frameCount - 1];
            ip = frame->ip;
            break;
        }
        case OP_CLOSURE: {
            ObjFunction* function = AS_FUNCTION(READ_CONSTANT());
            ObjClosure* closure = newClosure(function);
            PUSH_OBJ(closure);
            for (int i = 0; i < closure->upvalueCount; i++) {
                uint8_t isLocal = READ_BYTE();
                uint8_t index = READ_BYTE();
                closure->upvalues[i] = isLocal
                                           ? captureUpvalue(frame->slots + index)
                                           : getUpvalue(frame, index);
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
            PUSH_OBJ(newClass(READ_STRING()));
            break;
        }
        case OP_INHERIT: {
            Value superclass = peek(1);
            if (!IS_CLASS(superclass)) {
                frame->ip = ip;
                runtimeError("Superclass must be a class.");
                return INTERPRET_RUNTIME_ERROR;
            }
            ObjClass* subclass = AS_CLASS(peek(0));
            tableAddAll(
                &AS_CLASS(superclass)->methods,
                &subclass->methods);
            pop(); // Subclass
            break;
        }
        case OP_METHOD: {
            ObjString* name = READ_STRING();
            defineMethod(name);
            break;
        }
        case OP_STATIC_METHOD: {
            ObjString* name = READ_STRING();
            defineStaticMethod(name);
            break;
        }
        case OP_THROW: {
            frame->ip = ip;
            Value stacktrace = getStackTrace();
            Value value = peek(0);
            if (IS_INSTANCE(value)) {
                tableSet(
                    &AS_INSTANCE(value)->fields,
                    copyString("stackTrace", 10),
                    stacktrace);
            }
            if (propagateException()) {
                frame = &vm.frames[vm.frameCount - 1];
                ip = frame->ip;
                break;
            }
            return INTERPRET_RUNTIME_ERROR;
        }
        case OP_PUSH_EXCEPTION_HANDLER: {
            ObjString* typeName = READ_STRING();
            uint16_t handlerAddress = READ_SHORT();
            uint16_t finallyAddress = READ_SHORT();
            Value value;
            if (!tableGet(&vm.globals, typeName, &value) || !IS_CLASS(value)) {
                frame->ip = ip;
                runtimeError("Type '%s' is undefined in the global scope.", typeName->chars);
                return INTERPRET_RUNTIME_ERROR;
            }
            pushExceptionHandler(value, handlerAddress, finallyAddress);
            break;
        }
        case OP_POP_EXCEPTION_HANDLER: frame->handlerCount--;
            break;
        case OP_PROPAGATE_EXCEPTION: {
            frame->handlerCount--;
            frame->ip = ip;
            if (propagateException()) {
                frame = &vm.frames[vm.frameCount - 1];
                ip = frame->ip;
                break;
            }
            return INTERPRET_RUNTIME_ERROR;
        }
        }
    }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}

InterpretResult interpretCompiled(ObjFunction* function) {
    if (function == NULL) return INTERPRET_COMPILE_ERROR;
    PUSH_OBJ(function);
    callFunction((Obj*) function, 0);

    if (setjmp(vm.exit_state) == 0) {
        vm.exit_state_ready = true;
        return run();
    }

    return INTERPRET_EXIT;
}

InterpretResult interpret(InputFile source) {
    ObjFunction* function = compile(source);
    return interpretCompiled(function);
}
