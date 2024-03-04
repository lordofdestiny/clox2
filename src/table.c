//
// Created by djumi on 10/29/2023.
//

#include <stdlib.h>
#include <string.h>

#include "../h/memory.h"
#include "../h/object.h"
#include "../h/value.h"
#include "../h/table.h"

void initTable(Table* table) {
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void freeTable(Table* table) {
    FREE_ARRAY(Entry, table->entries, table->capacity);
    initTable(table);
}


static Entry* findEntry(Entry* entries, const int capacity, const ObjString* key) {
    uint32_t index = key->hash & (capacity - 1);
    Entry* tombstone = NULL;
    while (true) {
        Entry* entry = &entries[index];

        if (entry->key == NULL) {
            if (IS_NIL(entry->value)) {
                return tombstone != NULL ? tombstone : entry;
            }
            if (tombstone == NULL) {
                tombstone = entry;
            }
        } else if (entry->key == key) {
            // We found the key
            return entry;
        }

        index = (index + 1) & (capacity - 1);
    }
}


static void adjustCapacity(Table* table, const int capacity) {
    Entry* entries = ALLOCATE(Entry, capacity);
    if (entries == NULL) {
        fprintf(stderr, "Memory allocation failed: File: %s; Line: %d\n", __FILE__, __LINE__);
        exit(1);
    }
    for (int i = 0; i < capacity; i++) {
        entries[i] = (Entry){NULL, NIL_VAL};
    }

    table->count = 0;
    for (int i = 0; i < table->capacity; i++) {
        const Entry* entry = &table->entries[i];
        if (entry->key == NULL) continue;

        Entry* dest = findEntry(entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        table->count++;
    }
    FREE_ARRAY(Entry, table->entries, table->capacity);

    table->entries = entries;
    table->capacity = capacity;
}

bool tableGet(Table* table, ObjString* key, Value* value) {
    if (table->count == 0) return false;

    const Entry* entry = findEntry(table->entries, table->capacity, key);
    if (entry->key == NULL) return false;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedValue"
    *value = entry->value;
#pragma clang diagnostic pop

    return true;
}

bool tableSet(Table* table, ObjString* key, const Value value) {
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
        const int capacity = GROW_CAPACITY(table->capacity);
        adjustCapacity(table, capacity);
    }

    Entry* entry = findEntry(table->entries, table->capacity, key);
    const bool isNewKey = entry->key == NULL;
    if (isNewKey && IS_NIL(entry->value)) table->count++;

    entry->key = key;
    entry->value = value;
    return isNewKey;
}

bool tableDelete(Table* table, ObjString* key) {
    if (table->count == 0) return false;

    Entry* entry = findEntry(table->entries, table->capacity, key);
    if (entry->key == NULL) return false;

    entry->key = NULL;
    entry->value = BOOL_VAL(false);
    return true;
}

void tableAddAll(Table* from, Table* to) {
    for (int i = 0; i < from->capacity; i++) {
        const Entry* entry = &from->entries[i];
        if (entry->key != NULL) {
            tableSet(to, entry->key, entry->value);
        }
    }
}

ObjString* tableFindString(
    Table* table, const char* chars,
    const int length, const uint32_t hash
) {
    if (table->count == 0) return NULL;

    uint32_t index = hash & (table->capacity - 1);
    while (true) {
        const Entry* entry = &table->entries[index];
        if (entry->key == NULL) {
            // Stop if we find non-empty tombstone
            if (IS_NIL(entry->value)) return NULL;
        } else if (entry->key->length == length &&
                   entry->key->hash == hash &&
                   memcmp(entry->key->chars, chars, length) == 0) {

            return entry->key;
        }

        index = (index + 1) & (table->capacity - 1);
    }
}

ObjString* tableFindOrAddString(
    Table* table, const char* chars,
    const int length, const uint32_t hash
) {
    ObjString* string = tableFindString(table, chars, length, hash);
    if (string != NULL) return string;
    string = copyString(chars, length);
    tableSet(table, string, NIL_VAL);
    return string;
}

void tableRemoveWhite(Table* table) {
    for (int i = 0; i < table->capacity; i++) {
        const Entry* entry = &table->entries[i];
        if (entry->key != NULL && !entry->key->obj.isMarked) {
            tableDelete(table, entry->key);
        }
    }
}

void markTable(Table* table) {
    for (int i = 0; i < table->capacity; i++) {
        const Entry* entry = &table->entries[i];
        markObject((Obj*) entry->key);
        markValue(entry->value);
    }
}
