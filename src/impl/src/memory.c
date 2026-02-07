#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "common.h"
#include "memory.h"

#define GC_HEAP_GROW_FACTOR 2
#define NODE_VAULT_SIZE 16

struct GcHooks{
    PrintHookFnType printHook;
    BlackenHookFnType blackenHook;
    FreeHookFnType freeHook;

    size_t markRootsCount;
    MarkRootsHookFnType* markRootsHooks;

    size_t preSweepCount;
    PreSweepHookFnType* preSweepHooks;
};

typedef struct {
    GcHooks* hooks;

    size_t bytesAllocated;
    size_t nextGC;
    
    ilist_t objects;
    
    ilist_node_t** grayStack;
    int grayCount;
    int grayCapacity;

    GcNode* vault[NODE_VAULT_SIZE];
    size_t vaultPtr;
} GcState;

static GcState gcState;

[[maybe_unused]] static uint8_t gcContextCount = 0;

void initGcState(GcHooks* hooks) {
    gcState.hooks = hooks;

    gcState.bytesAllocated = 0;
    gcState.nextGC = 1024 * 1024;

    gcState.objects.head = ILIST_HEAD_INIT(gcState.objects.head);

    gcState.grayStack = NULL;
    gcState.grayCount = 0;
    gcState.grayCapacity = 0;

    memset(gcState.vault, 0, sizeof(gcState.vault));
    gcState.vaultPtr = 0;
}

void addToGcList(GcNode* gcNode) {
    ilist_add_front(&gcState.objects, &gcNode->node);
}

static void freeHooks(GcHooks* hooks) {
    free(hooks->markRootsHooks);
    free(hooks->preSweepHooks);
    free(hooks);
}

void freeGcState() {
    freeHooks(gcState.hooks);
    free(gcState.grayStack);
#ifdef DEBUG_LOG_GC
    fprintf(stdout, "%td bytes still allocated by the VM.\n", gcState.bytesAllocated);
#endif
}

static void collectGarbage();

void* reallocate(void* previous, const size_t oldSize, const size_t newSize) {
    gcState.bytesAllocated += newSize - oldSize;
    if (newSize > oldSize) {
#ifdef DEBUG_STRESS_GC
        collectGarbage();
#else
        if (gcState.bytesAllocated > gcState.nextGC) {
            collectGarbage();
        }
#endif
    }

    if (newSize == 0) {
        free(previous);
        return NULL;
    }

    void* result = realloc(previous, newSize);
    if (result == NULL) {
        fprintf(stderr, "Failed to allocate more memory\n");
        exit(1);
    }
    return result;
}

static void growGrayStack() {
    gcState.grayCapacity = GROW_CAPACITY(gcState.grayCapacity);
    ilist_node_t** newGrayStack = realloc(
        gcState.grayStack, 
        sizeof(newGrayStack[0]) * gcState.grayCapacity
    );

    // Shut down if it can't allocate more space for gray stack
    if (newGrayStack == NULL) {
        fprintf(stderr, "Could not grow GC grayStack\n");
        exit(1);
    }

    gcState.grayStack = newGrayStack;
}

void markGcNode(GcNode* gcNode) {
    if (gcNode == NULL) return;
    if (gcNode->isMarked) return;

#ifdef DEBUG_LOG_GC
    fprintf(stdout,"%p mark ", (void *) gcNode);
    gcState.hooks->printHook(gcNode);
    fprintf(stdout, "\n");
#endif
    gcNode->isMarked = true;

    if (gcState.grayCapacity < gcState.grayCount + 1) {
        growGrayStack();
    }

    gcState.grayStack[gcState.grayCount++] = &gcNode->node;
}

void freeObjects() {
    ilist_node_t* current = gcState.objects.head.next;

    while (current != &gcState.objects.head) {
        GcNode* gcNode = container_of(current, GcNode, node);
        current = current->next;

        ilist_remove(&gcNode->node);
        gcState.hooks->freeHook(gcNode);
    } 
}

static void traceReferences() {
    while (gcState.grayCount > 0) {
        ilist_node_t* node = gcState.grayStack[--gcState.grayCount];
        GcNode* gcNode = container_of(node, GcNode, node);
        gcState.hooks->blackenHook(gcNode);
    }
}

static void sweep() {
    for(size_t i = 0; i < gcState.hooks->preSweepCount; i++){
        gcState.hooks->preSweepHooks[i]();
    }

    ilist_node_t* node = gcState.objects.head.next;

    while (node != &gcState.objects.head) {
        GcNode* gcNode = container_of(node, GcNode, node);
        
        if (gcNode->isMarked) {
            gcNode->isMarked = false;
            node = node->next;
            continue;
        }

        node = node->next;

        ilist_remove(&gcNode->node);
        gcState.hooks->freeHook(gcNode);
    }
}

static void collectGarbage() {
#ifdef DEBUG_LOG_GC
    printf("-- gc begin\n");
    size_t before = gcState.bytesAllocated;
#endif

    // 1. Mark
    for(size_t i = 0; i < gcState.hooks->markRootsCount; i++) {
        gcState.hooks->markRootsHooks[i]();
    }
    // 2. Trace
    traceReferences();
    // 3. Sweep
    sweep();

    gcState.nextGC = gcState.bytesAllocated * GC_HEAP_GROW_FACTOR;

#ifdef DEBUG_LOG_GC
    fprintf(stdout, "-- gc end\n");
    fprintf(stdout, "   collected %zu bytes (from %zu to %zu) next at %zu\n",
           before - gcState.bytesAllocated, before, gcState.bytesAllocated,
           gcState.nextGC);
#endif
}

static int addMarkHookImpl(GcHooks* hooks, MarkRootsHookFnType markRootHook) {
    for(size_t i = 0; i < hooks->markRootsCount; i++) {
        if(hooks->markRootsHooks[i] == markRootHook) {
            return i;
        }
    }
    
    void* ptr = realloc(hooks->markRootsHooks, (hooks->markRootsCount + 1) * sizeof(*hooks->markRootsHooks));
    if (ptr == NULL) {
        free(hooks->markRootsHooks);
        free(hooks);
        fprintf(stderr, "Failed to register GC Mark Hook\n");
        exit(99);
    }
        
    size_t index = hooks->markRootsCount;

    hooks->markRootsHooks = ptr;
    hooks->markRootsHooks[hooks->markRootsCount++] = markRootHook;

    return index;
}

[[maybe_unused]] static int addPreSweepHookImpl(GcHooks* hooks, PreSweepHookFnType preSweepHook) {
    for(size_t i = 0; i < hooks->preSweepCount; i++) {
        if(hooks->preSweepHooks[i] == preSweepHook) {
            return i;
        }
    }
    
    void* ptr = realloc(hooks->preSweepHooks, (hooks->preSweepCount + 1) * sizeof(*hooks->preSweepHooks));
    if (ptr == NULL) {
        free(hooks->preSweepHooks);
        free(hooks);
        fprintf(stderr, "Failed to register GC Pre Sweep Hook\n");
        exit(99);
    }
        
    size_t index = hooks->preSweepCount;

    hooks->preSweepHooks = ptr;
    hooks->preSweepHooks[hooks->preSweepCount++] = preSweepHook;

    return index;
}

typedef void (*VoidFn)(void);

static int addHookImpl(VoidFn** hooksArray, size_t* hookCount, VoidFn hookPtr) {
    for(size_t i = 0; i < *hookCount; i++) {
        if((*hooksArray)[i] == hookPtr) {
            return i;
        }
    }

    void* ptr = realloc(*hooksArray, (*hookCount + 1) * sizeof(**hooksArray));
    if (ptr == NULL) {
        return -1;
    }

    size_t index = *hookCount;
    *hooksArray = ptr;
    (*hooksArray)[(*hookCount)++] = hookPtr;
    return index;
}

int addPreSweepHook(PreSweepHookFnType preSweepHook) {
    // GcHooks* hooks = gcState.hooks;
    // return addPreSweepHookImpl(hooks, preSweepHook);
    int ret = addHookImpl(&gcState.hooks->preSweepHooks, &gcState.hooks->preSweepCount, preSweepHook);
    if (ret == -1) {
        fprintf(stderr, "Failed to register GC Pre Sweep Hook\n");
        exit(99);
    }
    return ret;
}


int addMarkHook(MarkRootsHookFnType markRootHook) {
    // GcHooks* hooks = gcState.hooks;
    // return addMarkHookImpl(hooks, markRootHook);
    int ret = addHookImpl(&gcState.hooks->markRootsHooks, &gcState.hooks->markRootsCount, markRootHook);
    if (ret == -1) {
        fprintf(stderr, "Failed to register GC Pre Sweep Hook\n");
        exit(99);
    }
    return ret;
}

static void markVault();

GcHooks* createHooks(
    PrintHookFnType printHook,
    BlackenHookFnType blackenHook,
    FreeHookFnType freeHook
) {
    GcHooks* hooks = calloc(1, sizeof(*hooks));
    if (hooks == NULL) {
        return NULL;
    }

    hooks->printHook = printHook;
    hooks->blackenHook = blackenHook;
    hooks->freeHook = freeHook;

    hooks->markRootsCount = 0;
    hooks->markRootsHooks = NULL;

    hooks->preSweepCount = 0;
    hooks->preSweepHooks = NULL;

    addMarkHookImpl(hooks, markVault);

    return hooks;
}

static void markVault() {
    for(size_t i = 0 ; i < gcState.vaultPtr; i++) {
        markGcNode(gcState.vault[i]);
    }
}

int gcVaultPush(GcNode* gcNode) {
    if (gcState.vaultPtr >= 16) {
        return 1;
    }

    gcState.vault[gcState.vaultPtr++] = gcNode;

    return 0;
}

void gcVaultPop(size_t count) {
    if (count <= gcState.vaultPtr) {
        gcState.vaultPtr -= count;
        return;
    }
    
    gcState.vaultPtr = 0;
}
