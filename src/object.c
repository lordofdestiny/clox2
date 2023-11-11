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

static ObjVT vtList[];

static Obj *allocateObject(size_t size, ObjType type) {
    Obj *object = (Obj *) reallocate(NULL, 0, size);
    object->type = type;
    object->isMarked = false;
    object->vtp = &vtList[type];

    object->next = vm.objects;
    vm.objects = object;
    return object;
}

void printObject(Value value) {
    AS_OBJ(value)->vtp->print(AS_OBJ(value));
}

static void printFunctionImpl(ObjFunction *function);

ObjBoundMethod *newBoundMethod(Value receiver, Obj *method) {
    ObjBoundMethod *bound = ALLOCATE_OBJ(ObjBoundMethod, OBJ_BOUND_METHOD);
    bound->receiver = receiver;
    bound->method = method;
    return bound;
}

static void blackenBoundMethod(Obj *object) {
    ObjBoundMethod *bound = (ObjBoundMethod *) object;
    markValue(bound->receiver);
    markObject((Obj *) bound->method);
}

static void freeBoundMethod(Obj *object) {
    FREE(ObjBoundMethod, object);
}

static void printBoundMethod(Obj *obj) {
    Obj *method = ((ObjBoundMethod *) obj)->method;
    ObjFunction *fun = method->type == OBJ_FUNCTION
                       ? (ObjFunction *) method
                       : ((ObjClosure *) method)->function;
    printFunctionImpl(fun);
}

ObjClass *newClass(ObjString *name) {
    ObjClass *klass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
    klass->name = name;
    klass->initializer = NIL_VAL;
    initTable(&klass->methods);
    initTable(&klass->staticMethods);
    return klass;
}

static void blackenClass(Obj *object) {
    ObjClass *klass = (ObjClass *) object;
    markObject((Obj *) klass->name);
    markTable(&klass->methods);
    markTable(&klass->staticMethods);
}

static void freeClass(Obj *object) {
    ObjClass *class = (ObjClass *) object;
    freeTable(&class->methods);
    freeTable(&class->staticMethods);
    FREE(ObjClass, object);
}

static void printClass(Obj *obj) {
    printf("<class %s>", ((ObjClass *) obj)->name->chars);
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

static void blackenClosure(Obj *object) {
    ObjClosure *closure = (ObjClosure *) object;
    markObject((Obj *) closure->function);
    for (int i = 0; i < closure->upvalueCount; i++) {
        markObject((Obj *) closure->upvalues[i]);
    }
}

static void freeClosure(Obj *object) {
    ObjClosure *closure = (ObjClosure *) object;
    FREE_ARRAY(ObjClosure*, closure->upvalues, closure->upvalueCount);
    FREE(ObjClosure, object);
}

static void printClosure(Obj *obj) {
    printFunctionImpl(((ObjClosure *) obj)->function);
}

ObjFunction *newFunction() {
    ObjFunction *function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    function->arity = 0;
    function->upvalueCount = 0;
    function->name = NULL;
    initChunk(&function->chunk);
    return function;
}

static void blackenFunction(Obj *object) {
    ObjFunction *function = (ObjFunction *) object;
    markObject((Obj *) function->name);
    markArray(&function->chunk.constants);
}

static void freeFunction(Obj *object) {
    ObjFunction *function = (ObjFunction *) object;
    freeChunk(&function->chunk);
    FREE(ObjFunction, object);
}

static void printFunction(Obj *obj) {
    printFunctionImpl(((ObjFunction *) obj));
}

ObjInstance *newInstance(ObjClass *klass) {
    ObjInstance *instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
    instance->klass = klass;
    initTable(&instance->fields);
    return instance;
}

static void blackenInstance(Obj *object) {
    ObjInstance *instance = (ObjInstance *) object;
    markObject((Obj *) instance->klass);
    markTable(&instance->fields);
}

static void freeInstance(Obj *object) {
    ObjInstance *instance = (ObjInstance *) (object);
    freeTable(&instance->fields);
    FREE(ObjInstance, object);
}

static void printInstance(Obj *obj) {
    ObjInstance *instance = (ObjInstance *) obj;
    printf("<instance %s>", instance->klass->name->chars);
}

ObjNative *newNative(NativeFn function, int arity) {
    ObjNative *native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
    native->function = function;
    native->arity = arity;
    return native;
}

static void freeNative(Obj *object) {
    FREE(ObjNative, object);
}

static void printNative(Obj *obj) {
    // TODO see if this can be done in a way that displays more information
    printf("<native fn>");
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

static void freeString(Obj *object) {
    ObjString *string = (ObjString *) object;
    FREE_ARRAY(char, string->chars, (size_t) string->length + 1);
    FREE(ObjString, string);
}

static void printString(Obj *obj) {
    printf("%s", ((ObjString *) obj)->chars);
}

ObjUpvalue *newUpvalue(Value *slot) {
    ObjUpvalue *upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
    upvalue->location = slot;
    upvalue->next = NULL;
    upvalue->closed = NIL_VAL;
    return upvalue;
}

static void blackenUpvalue(Obj *object) {
    markValue(((ObjUpvalue *) object)->closed);
}

static void freeUpvalue(Obj *object) {
    FREE(ObjUpvalue, object);
}

static void printUpvalue(Obj *obj) {
    // TODO see if this can be done in a way that displays more information
    printf("upvalue");
}

static void printFunctionImpl(ObjFunction *function) {
    if (function->name == NULL) {
        printf("<script>");
        return;
    }
    printf("<fn %s>", function->name->chars);
}

static void blackenNoOp(Obj *obj) {}

static ObjVT vtList[] = {
        [OBJ_BOUND_METHOD] = {
                .call = callBoundMethod,
                .blacken = blackenBoundMethod,
                .free = freeBoundMethod,
                .print =printBoundMethod
        },
        [OBJ_CLASS] = {
                .call = callClass,
                .blacken = blackenClass,
                .free = freeClass,
                .print = printClass
        },
        [OBJ_CLOSURE] = {
                .call = callClosure,
                .blacken = blackenClosure,
                .free = freeClosure,
                .print = printClosure
        },
        [OBJ_FUNCTION] = {
                .call = callFunction,
                .blacken = blackenFunction,
                .free =freeFunction,
                .print = printFunction
        },
        [OBJ_INSTANCE] = {
                .call = NULL,
                .blacken = blackenInstance,
                .free = freeInstance,
                .print = printInstance
        },
        [OBJ_NATIVE] = {
                .call = callNative,
                .blacken = blackenNoOp,
                .free = freeNative,
                .print = printNative
        },
        [OBJ_STRING] = {
                .call = NULL,
                .blacken = blackenNoOp,
                .free = freeString,
                .print = printString
        },
        [OBJ_UPVALUE] = {
                .call = NULL,
                .blacken = blackenUpvalue,
                .free = freeUpvalue,
                .print = printUpvalue
        },
};

