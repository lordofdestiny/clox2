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

static ObjVT vts[];

static Obj *allocateObject(size_t size, ObjType type) {
    Obj *object = (Obj *) reallocate(NULL, 0, size);
    object->type = type;
    object->isMarked = false;
    object->vtp = &vts[type];

    object->next = vm.objects;
    vm.objects = object;
    return object;
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


ObjBoundMethod *newBoundMethod(Value receiver, Obj *method) {
    ObjBoundMethod *bound = ALLOCATE_OBJ(ObjBoundMethod, OBJ_BOUND_METHOD);
    bound->receiver = receiver;
    bound->method = method;
    return bound;
}

ObjClass *newClass(ObjString *name) {
    ObjClass *klass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
    klass->name = name;
    klass->initializer = NIL_VAL;
    initTable(&klass->methods);
    initTable(&klass->staticMethods);
    return klass;
}

ObjClosure *newClosure(ObjFunction *function) {
    ObjUpvalue **upvalues = ALLOCATE(ObjUpvalue*, function->upvalueCount);

    for (int i = 0; i < function->upvalueCount; i++) {
        upvalues[i] = NULL;
    }

    ObjClosure *closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
    closure->function = function;
    closure->upvalues = upvalues;
    closure->upvalueCount = function->upvalueCount;
    return closure;
}


ObjFunction *newFunction() {
    ObjFunction *function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    function->arity = 0;
    function->upvalueCount = 0;
    function->name = NULL;
    initChunk(&function->chunk);
    return function;
}

ObjInstance *newInstance(ObjClass *klass) {
    ObjInstance *instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
    instance->klass = klass;
    initTable(&instance->fields);
    return instance;
}

ObjNative *newNative(NativeFn function, int arity) {
    ObjNative *native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
    native->function = function;
    native->arity = arity;
    return native;
}

ObjString *takeString(char *chars, int length) {
    uint32_t hash = hashString(chars, length);
    ObjString *interned = tableFindString(&vm.strings, chars, length,
                                          hash);
    if (interned != NULL) {
        FREE_ARRAY(char, chars, (size_t) length + 1);
        return interned;
    }

    return allocateString(chars, length, hash);
}

ObjString *copyString(const char *chars, int length) {
    uint32_t hash = hashString(chars, length);
    ObjString *interned = tableFindString(&vm.strings, chars, length,
                                          hash);
    if (interned != NULL) return interned;

    char *heapChars = ALLOCATE(char, (size_t) length + 1);
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

void printObject(Value value) {
    AS_OBJ(value)->vtp->print(AS_OBJ(value));
}

static void printFunctionImpl(ObjFunction *function) {
    if (function->name == NULL) {
        printf("<script>");
        return;
    }
    printf("<fn %s>", function->name->chars);
}

static void printObjBoundMethod(Obj *obj) {
    Obj *method = ((ObjBoundMethod *) obj)->method;
    ObjFunction *fun = method->type == OBJ_FUNCTION
                       ? (ObjFunction *) method
                       : ((ObjClosure *) method)->function;
    printFunctionImpl(fun);
}

static void printObjClass(Obj *obj) {
    printf("<class %s>", ((ObjClass *) obj)->name->chars);
}

static void printObjClosure(Obj *obj) {
    printFunctionImpl(((ObjClosure *) obj)->function);
}

static void printObjFunction(Obj *obj) {
    printFunctionImpl(((ObjFunction *) obj));
}

static void printObjInstance(Obj *obj) {
    ObjInstance *instance = (ObjInstance *) obj;
    printf("<instance %s>", instance->klass->name->chars);
}

static void printObjNative(Obj *obj) {
    // TODO see if this can be done in a way that displays more information
    printf("<native fn>");
}

static void printObjString(Obj *obj) {
    printf("%s", ((ObjString *) obj)->chars);
}

static void printObjUpvalue(Obj *obj) {
    // TODO see if this can be done in a way that displays more information
    printf("upvalue");
}

static ObjVT vts[] = {
        [OBJ_BOUND_METHOD] = {callBoundMethod, NULL, printObjBoundMethod},
        [OBJ_CLASS] = {callClass, NULL, printObjClass},
        [OBJ_CLOSURE] = {callClosure, NULL, printObjClosure},
        [OBJ_FUNCTION] = {callFunction, NULL, printObjFunction},
        [OBJ_INSTANCE] = {NULL, NULL, printObjInstance},
        [OBJ_NATIVE] = {callNative, NULL, printObjNative},
        [OBJ_STRING] = {NULL, NULL, printObjString},
        [OBJ_UPVALUE] = {NULL, NULL, printObjUpvalue},
};

