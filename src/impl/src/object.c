#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"
#include "table.h"

#define ALLOCATE_OBJ(type, objectType) \
    (type*) allocateObject(sizeof(type), objectType)

static ObjVT vtList[];

static Obj* allocateObject(const size_t size, const ObjType type) {
    Obj* object = (Obj*) reallocate(NULL, 0, size);
    object->type = type;
    object->isMarked = false;
    object->vtp = &vtList[type];
    object->next = vm.objects;
    vm.objects = object;
    return object;
}

void printObject(FILE* out, const Value value) {
    AS_OBJ(value)->vtp->print(AS_OBJ(value), out);
}

static void printFunctionImpl(const ObjFunction* function, FILE* out);

ObjArray* newArray() {
    ObjArray* array = ALLOCATE_OBJ(ObjArray, OBJ_ARRAY);
    initValueArray(&array->array);
    return array;
}

static void blackenArray(Obj* object) {
    const ObjArray* array = (ObjArray*) object;
    for (int i = 0; i < array->array.count; i++) {
        markValue(array->array.values[i]);
    }
}

static void freeArray(Obj* object) {
    ObjArray* array = (ObjArray*) object;
    freeValueArray(&array->array);
    FREE(ObjArray, object);
}

static void printArray(Obj* object, FILE* out) {
    const ObjArray* objArray = (ObjArray*) object;
    const ValueArray* array = &objArray->array;
    fprintf(out, "[");
    for (int i = 0; i < array->count; i++) {
        if(IS_STRING(array->values[i])) {
            printf("\"");
            printValue(out, array->values[i]);
            printf("\"");
        }else {
            printValue(out, array->values[i]);
        }
        if (i + 1 != array->count) {
            printf(", ");
        }
    }
    printf("]");
}

ObjBoundMethod* newBoundMethod(const Value receiver, Obj* method) {
    ObjBoundMethod* bound = ALLOCATE_OBJ(ObjBoundMethod, OBJ_BOUND_METHOD);
    bound->receiver = receiver;
    bound->method = method;
    return bound;
}

static void blackenBoundMethod(Obj* object) {
    const ObjBoundMethod* bound = (ObjBoundMethod*) object;
    markValue(bound->receiver);
    markObject((Obj*) bound->method);
}

static void freeBoundMethod(Obj* object) {
    FREE(ObjBoundMethod, object);
}

static void printBoundMethod(Obj* obj, FILE* out) {
    Obj* method = ((ObjBoundMethod*) obj)->method;
    ObjFunction* fun = method->type == OBJ_FUNCTION
                           ? (ObjFunction*) method
                           : ((ObjClosure*) method)->function;
    printFunctionImpl(fun, out);
}

ObjClass* newClass(ObjString* name) {
    ObjClass* klass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
    klass->name = name;
    klass->initializer = NIL_VAL;
    initTable(&klass->fields);
    initTable(&klass->methods);
    initTable(&klass->staticMethods);
    return klass;
}

static void blackenClass(Obj* object) {
    ObjClass* klass = (ObjClass*) object;
    markObject((Obj*) klass->name);
    markTable(&klass->methods);
    markTable(&klass->staticMethods);
}

static void freeClass(Obj* object) {
    ObjClass* class = (ObjClass*) object;
    freeTable(&class->methods);
    freeTable(&class->staticMethods);
    FREE(ObjClass, object);
}

static void printClass(Obj* obj, FILE* out) {
    fprintf(out, "<class %s>", ((ObjClass*) obj)->name->chars);
}

ObjClosure* newClosure(ObjFunction* function) {
    ObjUpvalue** upvalues = ALLOCATE(ObjUpvalue*, function->upvalueCount);

    for (int i = 0; i < function->upvalueCount; i++) {
        upvalues[i] = NULL;
    }

    ObjClosure* closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
    closure->function = function;
    closure->upvalues = upvalues;
    closure->upvalueCount = function->upvalueCount;
    return closure;
}

static void blackenClosure(Obj* object) {
    const ObjClosure* closure = (ObjClosure*) object;
    markObject((Obj*) closure->function);
    for (int i = 0; i < closure->upvalueCount; i++) {
        markObject((Obj*) closure->upvalues[i]);
    }
}

static void freeClosure(Obj* object) {
    const ObjClosure* closure = (ObjClosure*) object;
    FREE_ARRAY(ObjClosure*, closure->upvalues, closure->upvalueCount);
    FREE(ObjClosure, object);
}

static void printClosure(Obj* obj, FILE* out) {
    printFunctionImpl(((ObjClosure*) obj)->function, out);
}

ObjFunction* newFunction() {
    ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    function->arity = 0;
    function->upvalueCount = 0;
    function->name = NULL;
    initChunk(&function->chunk);
    return function;
}

static void blackenFunction(Obj* object) {
    ObjFunction* function = (ObjFunction*) object;
    markObject((Obj*) function->name);
    markArray(&function->chunk.constants);
}

static void freeFunction(Obj* object) {
    ObjFunction* function = (ObjFunction*) object;
    freeChunk(&function->chunk);
    FREE(ObjFunction, object);
}

static void printFunction(Obj* obj, FILE* out) {
    printFunctionImpl(((ObjFunction*) obj), out);
}

ObjInstance* newInstance(ObjClass* klass) {
    ObjInstance* instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
    instance->klass = klass;
    instance->this_ = OBJ_VAL((Obj*) instance);
    initTable(&instance->fields);
    return instance;
}

static void blackenInstance(Obj* object) {
    ObjInstance* instance = (ObjInstance*) object;
    markValue(instance->this_);
    markObject((Obj*) instance->klass);
    markTable(&instance->fields);
}

static void freeInstance(Obj* object) {
    ObjInstance* instance = (ObjInstance*) (object);
    freeTable(&instance->fields);
    FREE(ObjInstance, object);
}

static void printInstance(Obj* obj, FILE* out) {
    const ObjInstance* instance = (ObjInstance*) obj;
    if (IS_INSTANCE(instance->this_)) {
        fprintf(out, "<instance %s>", instance->klass->name->chars);
    } else {
        printValue(out, instance->this_);
    }
}

ObjNative* newNative(const NativeFn function, const int arity) {
    ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
    native->function = function;
    native->arity = arity;
    return native;
}

static void freeNative(Obj* object) {
    FREE(ObjNative, object);
}

static void printNative(Obj* obj, FILE* out) {
    fprintf(out, "<native fn>");
}

ObjInstance* newPrimitive(const Value value, ObjClass* klass) {
    ObjInstance* primitive = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
    primitive->this_ = value;
    primitive->klass = klass;
    initTable(&primitive->fields);
    return primitive;
}

ObjString* allocateString(char* chars, const int length, const uint32_t hash) {
    ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;
    push(OBJ_VAL((Obj *) string));
    tableSet(&vm.strings, string, NIL_VAL);
    pop();
    return string;
}

uint32_t hashString(const char* chars, const int length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t) chars[i];
        hash *= 16777619;
    }
    return hash;
}


ObjString* takeString(char* chars, const int length) {
    const uint32_t hash = hashString(chars, length);
    ObjString* interned = tableFindString(
        &vm.strings, chars, length,
        hash);
    if (interned != NULL) {
        FREE_ARRAY(char, chars, (size_t) length + 1);
        return interned;
    }

    return allocateString(chars, length, hash);
}

ObjString* copyString(const char* chars, const int length) {
    const uint32_t hash = hashString(chars, length);
    ObjString* interned = tableFindString(
        &vm.strings, chars, length,
        hash);
    if (interned != NULL) return interned;

    char* heapChars = ALLOCATE(char, (size_t) length + 1);
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';
    return allocateString(heapChars, length, hash);
}

static void freeString(Obj* object) {
    const ObjString* string = (ObjString*) object;
    FREE_ARRAY(char, string->chars, (size_t) string->length + 1);
    FREE(ObjString, object);
}

static void printString(Obj* obj, FILE* out) {
    fprintf(out, "%s", ((ObjString*) obj)->chars);
}

ObjUpvalue* newUpvalue(Value* slot) {
    ObjUpvalue* upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
    upvalue->location = slot;
    upvalue->next = NULL;
    upvalue->closed = NIL_VAL;
    return upvalue;
}

static void blackenUpvalue(Obj* object) {
    markValue(((ObjUpvalue*) object)->closed);
}

static void freeUpvalue(Obj* object) {
    FREE(ObjUpvalue, object);
}

static void printUpvalue(Obj* obj, FILE* out) {
    const ObjUpvalue* upvalue = (ObjUpvalue*) obj;
    fprintf(out, "<upvalue ");
    printValue(out, *upvalue->location);
    fprintf(out, ">");
}

static void printFunctionImpl(const ObjFunction* function, FILE* out) {
    if (function->name == NULL) {
        printf("<script>");
        return;
    }
    fprintf(out, "<fn %s>", function->name->chars);
}

static bool callNonCallable(Obj* obj, int argCount) {
    runtimeError("Can only call functions and classes.");
    return false;
}

static void blackenNoOp(Obj* obj) { }

static ObjVT vtList[] = {
    [OBJ_ARRAY] = {
        .call = callNonCallable,
        .blacken = blackenArray,
        .free = freeArray,
        .print = printArray
    },
    [OBJ_BOUND_METHOD] = {
        .call = callBoundMethod,
        .blacken = blackenBoundMethod,
        .free = freeBoundMethod,
        .print = printBoundMethod
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
        .free = freeFunction,
        .print = printFunction
    },
    [OBJ_INSTANCE] = {
        .call = callNonCallable,
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
        .call = callNonCallable,
        .blacken = blackenNoOp,
        .free = freeString,
        .print = printString
    },
    [OBJ_UPVALUE] = {
        .call = callNonCallable,
        .blacken = blackenUpvalue,
        .free = freeUpvalue,
        .print = printUpvalue
    },
};

