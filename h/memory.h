//
// Created by djumi on 10/29/2023.
//

#ifndef CLOX2_MEMORY_H
#define CLOX2_MEMORY_H

#include "common.h"
#include "object.h"

#define ALLOCATE(type, count) \
    (type*) reallocate(NULL, 0, sizeof(type)*(count))

#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

#define GROW_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_ARRAY(type, previous, oldCount, count) \
    (type *)reallocate(previous, sizeof(type) * (oldCount), sizeof(type) * (count))

#define FREE_ARRAY(type, pointer, oldCount) \
    (type *)reallocate(pointer, sizeof(type) * (oldCount), 0)

void *reallocate(void *previous, size_t oldSize, size_t newSize);

void markObject(Obj *object);

void markValue(Value value);

void collectGarbage();

void freeObjects();

#endif //CLOX2_MEMORY_H
