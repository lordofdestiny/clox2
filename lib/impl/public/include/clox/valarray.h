#ifndef __CLOX2_VALUE_ARRAY_H__
#define __CLOX2_VALUE_ARRAY_H__

#include <stdio.h>

#include <clox/export.h>

#include <clox/value.h>

typedef struct {
    int capacity;
    int count;
    Value* values;
} ValueArray;

CLOX_EXPORT void initValueArray(ValueArray* array);

CLOX_EXPORT void writeValueArray(ValueArray* array, Value value);

CLOX_EXPORT void copyValueArray(ValueArray* src, ValueArray* dest);

CLOX_EXPORT void valueInitValueArray(ValueArray* array, Value initial, int count);

CLOX_EXPORT void freeValueArray(ValueArray* array);

CLOX_EXPORT void printValue(FILE* out, Value value);

#endif // __CLOX2_VALUE_ARRAY_H__
