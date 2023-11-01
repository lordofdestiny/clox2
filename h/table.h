//
// Created by djumi on 10/29/2023.
//

#ifndef CLOX2_TABLE_H
#define CLOX2_TABLE_H

#include "value.h"

#define TABLE_MAX_LOAD 0.75

typedef struct {
    ObjString *key;
    Value value;
} Entry;

typedef struct {
    int count;
    int capacity;
    Entry *entries;
} Table;

void initTable(Table *table);

void freeTable(Table *table);

bool tableGet(Table *table, ObjString *key, Value *value);

bool tableSet(Table *table, ObjString *key, Value value);

bool tableDelete(Table *table, ObjString *key);

void tableAddAll(Table *from, Table *to);

ObjString *tableFindString(Table *table, const char *chars, int length, uint32_t hash);

void tableRemoveWhite(Table *table);

void markTable(Table *table);

#endif //CLOX2_TABLE_H
