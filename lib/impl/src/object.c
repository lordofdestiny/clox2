#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <clox/table.h>
#include <clox/value.h>
#include <clox/vm.h>

#include <impl/chunk.h>
#include <impl/memory.h>
#include <impl/object.h>
#include <impl/vm.h>


#define IS_BUILTIN_OBJ_TYPE(objectType) (objectType > OBJ_NONE && objectType < OBJ_END)

#define ALLOCATE_OBJ(type, objectType, ...) \
    (type*) allocateObject(sizeof(type), objectType, \
        __VA_OPT__(IS_BUILTIN_OBJ_TYPE(objectType)) \
        __VA_OPT__(?)    &vtList[objectType] \
        __VA_OPT__(: __VA_ARGS__) \
    )

static bool callNonCallable(Obj*, int);
static void blackenNoOp(Obj*);
static void printFunctionImpl(const ObjFunction* function, FILE* out);

static void blackenArray(Obj* object);
static void freeArray(Obj* object);
static void printArray(Obj* object, FILE* out);

static void blackenBoundMethod(Obj* object);
static void freeBoundMethod(Obj* object);
static void printBoundMethod(Obj* obj, FILE* out);

static void blackenClass(Obj* object);
static void freeClass(Obj* object);
static void printClass(Obj* obj, FILE* out);

static void blackenClosure(Obj* object);
static void freeClosure(Obj* object);
static void printClosure(Obj* obj, FILE* out);

static void blackenFunction(Obj* object);
static void freeFunction(Obj* object);
static void printFunction(Obj* obj, FILE* out);

static void blackenInstance(Obj* object);
static void freeInstance(Obj* object);
static void printInstance(Obj* obj, FILE* out);

static void freeNative(Obj* object);
static void printNative(Obj* obj, FILE* out);

static void freeString(Obj* object);
static void printString(Obj* obj, FILE* out);

static void blackenUpvalue(Obj* object);
static void freeUpvalue(Obj* object);
static void printUpvalue(Obj* obj, FILE* out);

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

static Obj* allocateObject(const size_t size, const ObjType type, ObjVT* vtp) {
    Obj* object = (Obj*) reallocate(NULL, 0, size);
    object->type = type;
    object->isMarked = false;
    object->vtp = vtp;
    object->next = vm.objects;
    vm.objects = object;
    return object;
}

void printObject(FILE* out, const Value value) {
    AS_OBJ(value)->vtp->print(AS_OBJ(value), out);
}

ObjArray* newArray() {
    ObjArray* array = ALLOCATE_OBJ(ObjArray, OBJ_ARRAY);
    PUSH_OBJ(array);
    initValueArray(&array->array);
    pop();
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
         if (IS_STRING(array->values[i])) {
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

ObjNative* newNative(const char* name, const NativeFn function, const int arity) {
    ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
    native->name = name;
    native->function = function;
    native->arity = arity;
    return native;
}

static void freeNative(Obj* object) {
    FREE(ObjNative, object);
}

static void printNative([[maybe_unused]] Obj* obj, FILE* out) {
    fprintf(out, "<native fn %s>", ((ObjNative*)obj)->name);
}

ObjInstance* newPrimitive(const Value value, ObjClass* klass) {
    ObjInstance* primitive = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
    primitive->this_ = value;
    primitive->klass = klass;
    initTable(&primitive->fields);
    return primitive;
}

static ObjString* allocateString(char* chars, const int length, const uint32_t hash) {
    ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;
    PUSH_OBJ(string);
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

ObjString* escapedString(const char* chars, int length) {
    char* buffer = ALLOCATE(char, (size_t) length + 1);
    int write = 0;
    int read = 0;
    while(read < length) {
        char c = chars[read++];
        if (c != '\\') {
            buffer[write++] = c;
            continue;
        }

        if (isalpha(c = chars[read]) || c == '\'' || c == '\"' || c == '\\') {
            switch (chars[read++]) {
            case 'a': buffer[write++] = '\a'; continue;
            case 'b': buffer[write++] = '\b'; continue;
            case 'f': buffer[write++] = '\f'; continue;
            case 'r': buffer[write++] = '\r'; continue;
            case 'n': buffer[write++] = '\n'; continue;
            case 't': buffer[write++] = '\t'; continue;
            case 'v': buffer[write++] = '\v'; continue;
            case '?': buffer[write++] = '?'; continue;
            case '\\': buffer[write++] = '\\'; continue;
            case '\'': buffer[write++] = '\''; continue;
            case '\"': buffer[write++] = '\"'; continue;
            case 'x': {
                char total = 0;
                int i = 0;
                while(isdigit(c = chars[read]) && i < 2) {
                    total = 16 * total + (c-'0');
                    read++;
                    i++;
                }
                buffer[write++] = total;
                continue;
            }
            default:
            continue;
            }
        }else {
            char total = 0;
            int i = 0;
            while(isdigit(c = chars[read]) && i < 3) {
                total = 8 * total + (c-'0');
                read++;
                i++;
            }
            buffer[write++] = total;
        }  
    }
    buffer[write] = 0;
    
    return takeString(buffer, write);
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
        fprintf(out, "<script>");
        return;
    }
    fprintf(out, "<fn %s>", function->name->chars);
}

static bool callNonCallable([[maybe_unused]] Obj* obj, [[maybe_unused]] int argCount) {
    runtimeError("Can only call functions and classes.");
    return false;
}

static void blackenNoOp([[maybe_unused]] Obj* obj) { }
