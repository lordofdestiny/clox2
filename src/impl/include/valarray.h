#ifndef __CLOX2_VALUE_ARRAY_H__
#define __CLOX2_VALUE_ARRAY_H__

#include <stdio.h>

#include "clox/value.h"

typedef struct {
    int capacity;
    int count;
    Value* values;
} ValueArray;

bool valuesEqual(Value a, Value b);

void initValueArray(ValueArray* array);

void writeValueArray(ValueArray* array, Value value);

void copyValueArray(ValueArray* src, ValueArray* dest);

void valueInitValueArray(ValueArray* array, Value initial, int count);

void freeValueArray(ValueArray* array);

void printValue(FILE* out, Value value);

#endif // __CLOX2_VALUE_ARRAY_H__
