#include <stdlib.h>

#include "common.h"
#include "memory.h"
#include "vm.h"
#include "compiler.h"

#ifdef DEBUG_LOG_GC

#include <stdio.h>
#include "debug.h"

#endif
#define GC_HEAP_GROW_FACTOR 2

void* reallocate(void* previous, const size_t oldSize, const size_t newSize) {
    vm.bytesAllocated += newSize - oldSize;
    if (newSize > oldSize) {
#ifdef DEBUG_STRESS_GC
        collectGarbage();
#else
        if (vm.bytesAllocated > vm.nextGC) {
            collectGarbage();
        }
#endif
    }


    if (newSize == 0) {
        free(previous);
        return NULL;
    }
    void* result = realloc(previous, newSize);
    if (result == NULL) exit(1);
    return result;
}

void markObject(Obj* object) {
    if (object == NULL) return;
    if (object->isMarked) return;

#ifdef DEBUG_LOG_GC
    printf("%p mark ", (void *) object);
    printValue(OBJ_VAL(object));
    printf("\n");
#endif
    object->isMarked = true;

    if (vm.grayCapacity < vm.grayCount + 1) {
        vm.grayCapacity = GROW_CAPACITY(vm.grayCapacity);
        Obj** newGrayStack = (Obj**) realloc(vm.grayStack, sizeof(Obj*) * vm.grayCapacity);

        // Shut down if it can't allocate more space for gray stack
        if (newGrayStack == NULL) {
            free(vm.grayStack);
            exit(1);
        }

        vm.grayStack = newGrayStack;
    }

    vm.grayStack[vm.grayCount++] = object;
}

void markValue(const Value value) {
    if (IS_OBJ(value)) markObject(AS_OBJ(value));
}

void markArray(ValueArray* array) {
    for (int i = 0; i < array->count; i++) {
        markValue(array->values[i]);
    }
}

static void blackenObject(Obj* object) {
#ifdef DEBUG_LOG_GC
    printf("%p blacken ", (void *) object);
    printValue(OBJ_VAL(object));
    printf("\n");
#endif
    object->vtp->blacken(object);
}

static void freeObject(Obj* object) {
#ifdef DEBUG_LOG_GC
    printf("%p free type %d\n", (void *) object, object->type);
#endif
    object->vtp->free(object);
}

void freeObjects() {
    Obj* object = vm.objects;
    while (object != NULL) {
        Obj* next = object->next;
        freeObject(object);
        object = next;
    }
}

static void markRoots() {
    // Mark objects on the VM stack
    for (const Value* slot = vm.stack; slot <= vm.stackTop; slot++) {
        markValue(*slot);
    }

    for (int i = 0; i < vm.frameCount; i++) {
        markObject((Obj*) vm.frames[i].function);
    }

    markTable(&vm.globals);

    for (ObjUpvalue* upvalue = vm.openUpvalues;
         upvalue != NULL;
         upvalue = upvalue->next) {
        markObject((Obj*) upvalue);
    }

    markCompilerRoots();
    markObject((Obj*) vm.initString);
}

static void traceReferences() {
    while (vm.grayCount > 0) {
        Obj* object = vm.grayStack[--vm.grayCount];
        blackenObject(object);
    }
}

static void sweep() {
    Obj* previous = NULL;
    Obj* object = vm.objects;

    while (object != NULL) {
        if (object->isMarked) {
            object->isMarked = false;
            previous = object;
            object = object->next;
            continue;
        }

        Obj* unreached = object;
        object = object->next;
        if (previous != NULL) {
            previous->next = object;
        } else {
            vm.objects = object;
        }

        freeObject(unreached);

    }
}

void collectGarbage() {
#ifdef DEBUG_LOG_GC
    printf("-- gc begin\n");
    size_t before = vm.bytesAllocated;
#endif

    markRoots();
    traceReferences();
    tableRemoveWhite(&vm.strings);
    sweep();

    vm.nextGC = vm.bytesAllocated * GC_HEAP_GROW_FACTOR;

#ifdef DEBUG_LOG_GC
    printf("-- gc end\n");
    printf("   collected %zu bytes (from %zu to %zu) next at %zu\n",
           before - vm.bytesAllocated, before, vm.bytesAllocated,
           vm.nextGC);
#endif
}
