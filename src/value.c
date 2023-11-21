//
// Created by djumi on 10/29/2023.
//

#include <stdio.h>
#include <string.h>

#include "../h/memory.h"
#include "../h/value.h"

bool valuesEqual(Value a, Value b) {
#ifdef NAN_BOXING
    if (IS_NUMBER(a) && IS_NUMBER(b)) {
        return AS_NUMBER(a) == AS_NUMBER(b);
    }
    return a == b;
#else
    if (a.type != b.type) return false;
    switch (a.type) {
    case VAL_BOOL: return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NIL: return true;
    case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
    case VAL_OBJ: return AS_OBJ(a) == AS_OBJ(b);
    default: return false; // Unreachable
    }
#endif
}

void initValueArray(ValueArray *array) {
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

void writeValueArray(ValueArray *array, Value value) {
    if (array->capacity < array->count + 1) {
        int oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->values = GROW_ARRAY(Value, array->values, oldCapacity, array->capacity);
    }

    array->values[array->count] = value;
    array->count++;
}

void freeValueArray(ValueArray *array) {
    FREE_ARRAY(Value, array->values, array->capacity);
    initValueArray(array);
}

#ifdef NAN_BOXING

void printValue(FILE *out, Value value) {
    if (IS_BOOL(value)) {
        fprintf(out, AS_BOOL(value) ? "true" : "false");
    } else if (IS_NIL(value)) {
        fprintf(out, "nil");
    } else if (IS_NUMBER(value)) {
        fprintf(out, "%g", AS_NUMBER(value));
    } else if (IS_OBJ(value)) {
        printObject(out, value);
    }
}

#else
void printValue(Value value) {
    switch (value.type) {
    case VAL_BOOL:printf(AS_BOOL(value) ? "true" : "false");
        break;
    case VAL_NIL: printf("nil");
        break;
    case VAL_NUMBER: printf("%g", AS_NUMBER(value));
        break;
    case VAL_OBJ: printObject(value);
        break;
    }
}
#endif