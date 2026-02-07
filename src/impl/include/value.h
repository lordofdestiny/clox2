#ifndef __CLOX2_VALUE_H__
#define __CLOX2_VALUE_H__

#include <stdio.h>
#include <string.h>
#include "common.h"

typedef struct Obj Obj;
typedef struct ObjString ObjString;

#ifdef NAN_BOXING

#define SIGN_BIT ((uint64_t)0x8000000000000000)
#define QNAN ((uint64_t)0x7ffc000000000000)

#define TAG_NIL 1 // 01
#define TAG_FALSE  2 // 10
#define TAG_TRUE 3 // 11

typedef uint64_t Value;

#define IS_BOOL(value) (((value) | 1) == TRUE_VAL)
#define IS_NIL(value) ((value) == NIL_VAL)
#define IS_NUMBER(value) (((value) & QNAN) != QNAN)
#define IS_OBJ(value) \
    (((value) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))

#define AS_BOOL(value) (value == TRUE_VAL)
#define AS_NUMBER(value) valueToNum(value)
#define AS_OBJ(value) \
    ((Obj*)(uintptr_t)((value) & ~(SIGN_BIT | QNAN)))

#define BOOL_VAL(b) ((b) ? TRUE_VAL: FALSE_VAL)
#define FALSE_VAL ((Value)(uint64_t)(QNAN | TAG_FALSE))
#define NIL_VAL ((Value)(uint64_t)(QNAN | TAG_NIL))
#define TRUE_VAL ((Value)(uint64_t)(QNAN | TAG_TRUE))
#define NUMBER_VAL(num) numToValue(num)
#define OBJ_VAL(obj) \
    (Value) (SIGN_BIT | QNAN | (uint64_t)(uintptr_t)(obj))


static inline double valueToNum(const Value value) {
    double num;
    memcpy(&num, &value, sizeof(double));
    return num;
}

static inline Value numToValue(const double num) {
    Value value;
    memcpy(&value, &num, sizeof(double));
    return value;
}

#else
typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_OBJ,
} ValueType;

typedef struct {
    ValueType type;

    union {
        bool boolean;
        double number;
        Obj* obj;
    } as;
} Value;

#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)
#define IS_OBJ(value) ((value).type == VAL_OBJ)

#define AS_OBJ(value) ((value).as.obj)
#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)

#define BOOL_VAL(value) ((Value){VAL_BOOL, {.boolean = value}})
#define TRUE_VAL BOOL_VAL(true)
#define FALSE_VAL BOOL_VAL(false)
#define NIL_VAL ((Value){VAL_NIL, {.number = 0}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})
#define OBJ_VAL(value) ((Value) {VAL_OBJ, {.obj = value}})

#endif


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

void markValue(const Value value);

void markArray(ValueArray* array);

#endif //__CLOX2_VALUE_H__
