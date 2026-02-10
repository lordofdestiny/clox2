#include <stdio.h>
#include <string.h>

#include "object.h"
#include "clox/value.h"
#include "memory.h"

bool valuesEqual(Value a, Value b) {
    if (IS_INSTANCE(a) && !IS_INSTANCE(AS_INSTANCE(a)->this_)) {
        a = AS_INSTANCE(a)->this_;
    }
    if (IS_INSTANCE(b) && !IS_INSTANCE(AS_INSTANCE(b)->this_)) {
        b = AS_INSTANCE(b)->this_;
    }
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

void initValueArray(ValueArray* array) {
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

void writeValueArray(ValueArray* array, const Value value) {
    if (array->capacity < array->count + 1) {
        const int oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->values = GROW_ARRAY(Value, array->values, oldCapacity, array->capacity);
    }

    array->values[array->count] = value;
    array->count++;
}

static int nextPowerOfTwo(int n) {
    int i = 0;
    for (--n; n > 0; n >>= 1) {
        i++;
    }
    return 1 << i;
}

void valueInitValueArray(ValueArray* array, const Value initial, const int count) {
    if (array->capacity < count) {
        const int oldCapacity = array->capacity;
        array->capacity = nextPowerOfTwo(count);
        array->values = GROW_ARRAY(Value, array->values, oldCapacity, array->capacity);
    }
    for (int i = 0; i < count; i++) {
        array->values[i] = initial;
    }
    array->count = count;
}

void copyValueArray(ValueArray* src, ValueArray* dest) {
    if (src->capacity > dest->capacity) {
        // Grow dest to size of capacity
        const int oldCapacity = dest->capacity;
        dest->capacity = src->capacity;
        dest->values = GROW_ARRAY(Value, dest->values, oldCapacity, dest->capacity);
    }

    for (int i = 0; i < src->count; i++) {
        dest->values[i] = src->values[i];
    }
    dest->count = src->count;
}

void freeValueArray(ValueArray* array) {
    FREE_ARRAY(Value, array->values, array->capacity);
    initValueArray(array);
}

#ifdef NAN_BOXING

void printValue(FILE* out, const Value value) {
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
void printValue(FILE* out, const Value value) {
    switch (value.type) {
    case VAL_BOOL: fprintf(out, AS_BOOL(value) ? "true" : "false");
        break;
    case VAL_NIL: fprintf(out, "nil");
        break;
    case VAL_NUMBER: fprintf(out, "%g", AS_NUMBER(value));
        break;
    case VAL_OBJ: printObject(out, value);
        break;
    }
}
#endif
