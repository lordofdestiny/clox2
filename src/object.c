//
// Created by djumi on 10/29/2023.
//

#include <stdio.h>
#include <string.h>

#include "../h/memory.h"
#include "../h/object.h"
#include "../h/value.h"
#include "../h/vm.h"
#include "../h/table.h"

#define ALLOCATE_OBJ(type, objectType) \
    (type*) allocateObject(sizeof(type), objectType)

#define ALLOCATE_CALLABLE(type, objectType, caller) \
    (type*) allocateCallable(sizeof(type), objectType, caller)

static void initObject(Obj *object, ObjType type, bool callable) {
    object->type = type;
    object->isMarked = false;
    object->isCallable = callable;
    object->next = vm.objects;
    vm.objects = object;
}

static Obj *allocateObject(size_t size, ObjType type) {
    Obj *object = (Obj *) reallocate(NULL, 0, size);
    initObject(object, type, false);
    return object;
}

static Callable *allocateCallable(size_t size, ObjType type, CallableFn caller) {
    Callable *callable = (Callable *) reallocate(NULL, 0, size);
    initObject((Obj *) callable, type, true);
    callable->caller = caller;
    return callable;
}

static ObjString *allocateString(char *chars, int length,
                                 uint32_t hash) {
    ObjString *string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;
    push(OBJ_VAL((Obj *) string));
    tableSet(&vm.strings, string, NIL_VAL);
    pop();
    return string;
}

uint32_t hashString(const char *key, int length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t) key[i];
        hash *= 16777619;
    }
    return hash;
}

static bool callClosure(Callable *callable, int argCount) {
    return callNonNative((Obj *) callable, ((ObjClosure *) callable)->function, argCount);
}

ObjClosure *newClosure(ObjFunction *function) {
    ObjUpvalue **upvalues = ALLOCATE(ObjUpvalue*, function->upvalueCount);

    for (int i = 0; i < function->upvalueCount; i++) {
        upvalues[i] = NULL;
    }

    ObjClosure *closure = ALLOCATE_CALLABLE(ObjClosure, OBJ_CLOSURE, callClosure);
    closure->function = function;
    closure->upvalues = upvalues;
    closure->upvalueCount = function->upvalueCount;
    return closure;
}

static bool callFunction(Callable *callable, int argCount) {
    return callNonNative((Obj *) callable, (ObjFunction *) callable, argCount);
}

ObjFunction *newFunction() {
    ObjFunction *function = ALLOCATE_CALLABLE(ObjFunction, OBJ_FUNCTION, callFunction);
    function->arity = 0;
    function->upvalueCount = 0;
    function->name = NULL;
    initChunk(&function->chunk);
    return function;
}

static bool callNative(Callable *callable, int argCount) {
    ObjNative *native = (ObjNative *) callable;
    if (argCount != native->arity) {
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

ObjNative *newNative(NativeFn function, int arity) {
    ObjNative *native = ALLOCATE_CALLABLE(ObjNative, OBJ_NATIVE, callNative);
    native->function = function;
    native->arity = arity;
    return native;
}

ObjString *takeString(char *chars, int length) {
    uint32_t hash = hashString(chars, length);
    ObjString *interned = tableFindString(&vm.strings, chars, length,
                                          hash);
    if (interned != NULL) {
        FREE_ARRAY(char, chars, length + 1);
        return interned;
    }

    return allocateString(chars, length, hash);
}

ObjString *copyString(const char *chars, int length) {
    uint32_t hash = hashString(chars, length);
    ObjString *interned = tableFindString(&vm.strings, chars, length,
                                          hash);
    if (interned != NULL) return interned;

    char *heapChars = ALLOCATE(char, length + 1);
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';
    return allocateString(heapChars, length, hash);
}

ObjUpvalue *newUpvalue(Value *slot) {
    ObjUpvalue *upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
    upvalue->location = slot;
    upvalue->next = NULL;
    upvalue->closed = NIL_VAL;
    return upvalue;
}

static void printFunction(ObjFunction *function) {
    if (function->name == NULL) {
        printf("<script>");
        return;
    }
    printf("<fn %s>", function->name->chars);
}


void printObject(Value value) {
    switch (OBJ_TYPE(value)) {
    case OBJ_CLOSURE: printFunction(AS_CLOSURE(value)->function);
        break;
    case OBJ_FUNCTION: printFunction(AS_FUNCTION(value));
        break;
    case OBJ_NATIVE: printf("<native fn>");
        break;
    case OBJ_STRING: printf("%s", AS_CSTRING(value));
        break;
    case OBJ_UPVALUE:printf("upvalue");
        break;
    }
}