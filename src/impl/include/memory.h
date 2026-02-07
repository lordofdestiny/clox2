#ifndef __CLOX2_MEMORY_H__
#define __CLOX2_MEMORY_H__

#include <stdio.h>

#include "ilist.h"

typedef struct GcObjNode GcNode;

struct GcObjNode{
    ilist_node_t node;
    bool isMarked;
};

typedef void (*MarkRootsHookFnType)();
typedef void (*PreSweepHookFnType)();
typedef void (*PrintHookFnType)(GcNode*);
typedef void (*BlackenHookFnType)(GcNode*);
typedef void (*FreeHookFnType)(GcNode*);

typedef struct GcHooks GcHooks;

#define ALLOCATE(type, count) \
    (type*) reallocate(NULL, 0, sizeof(type)*(count))

#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

#define GROW_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_ARRAY(type, previous, oldCount, count) \
    (type *)reallocate(previous, sizeof(type) * (oldCount), sizeof(type) * (count))

#define FREE_ARRAY(type, pointer, oldCount) \
    (type *)reallocate(pointer, sizeof(type) * (oldCount), 0)

void* reallocate(void* previous, size_t oldSize, size_t newSize);

GcHooks* createHooks(
    PrintHookFnType printHook,
    BlackenHookFnType blackenHook,
    FreeHookFnType freeHook
);

int addMarkHook(MarkRootsHookFnType markRootHook);

int addPreSweepHook(PreSweepHookFnType preSweepHook);

void initGcState(GcHooks* hooks);

void freeGcState();

void addToGcList(GcNode* gcNode);

void markGcNode(GcNode* gcNode);

void freeObjects();

int gcVaultPush(GcNode* gcNode);

void gcVaultPop(size_t count);

#endif //__CLOX2_MEMORY_H__
