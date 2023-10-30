//
// Created by djumi on 10/29/2023.
//

#include <stdlib.h>

#include "../h/common.h"
#include "../h/memory.h"
#include "../h/vm.h"

void *reallocate(void *previous, size_t oldSize, size_t newSize) {
    if (newSize == 0) {
        free(previous);
        return NULL;
    }
    void *result = realloc(previous, newSize);
    if (result == NULL) exit(1);
    return result;
}

static void freeObject(Obj *object) {
    switch (object->type) {
    case OBJ_STRING: {
        ObjString *string = (ObjString *) object;
        reallocate(object, sizeof(ObjString) + string->length + 1, 0);
    }
    default: return; // Unreachable
    }
}

void freeObjects() {
    Obj *object = vm.objects;
    while (object != NULL) {
        Obj *next = object->next;
        freeObject(object);
        object = next;
    }
}