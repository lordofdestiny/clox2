#ifndef __CLOX2_TABLE_H__
#define __CLOX2_TABLE_H__

#include <clox/export.h>

#include <clox/value.h>

typedef struct ObjString ObjString;

#define TABLE_MAX_LOAD 0.75

typedef struct {
    ObjString* key;
    Value value;
} Entry;

typedef struct {
    int count;
    int capacity;
    Entry* entries;
} Table;

typedef struct {
    Table* table;
    int index;
    bool done;
} TableIterator;

CLOX_EXPORT void initTable(Table* table);

CLOX_EXPORT void freeTable(Table* table);

CLOX_EXPORT bool tableGet(Table* table, ObjString* key, Value* value);

CLOX_EXPORT bool tableSet(Table* table, ObjString* key, Value value);

CLOX_EXPORT bool tableDelete(Table* table, ObjString* key);

CLOX_EXPORT void tableAddAll(Table* from, Table* to);

CLOX_NO_EXPORT ObjString* tableFindString(
    Table* table, const char* chars,
    int length, uint32_t hash
);

CLOX_NO_EXPORT ObjString* tableFindOrAddString(
    Table* table, const char* chars,
    int length, uint32_t hash
);

CLOX_NO_EXPORT void tableRemoveWhite(Table* table);

CLOX_NO_EXPORT void markTable(Table* table);

CLOX_EXPORT TableIterator newTableIterator(Table* table);

CLOX_EXPORT void advanceTableIterator(TableIterator* it);

CLOX_EXPORT ObjString* getKeyTableIterator(TableIterator* it);

CLOX_EXPORT Value getValueTableIterator(TableIterator* it);

#endif //__CLOX2_TABLE_H__
