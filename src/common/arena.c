#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <stdio.h>

#include "arena.h"

#define STATIC_BUFFER_BYTES 1024 * 1024 // 1MB

#define ALIGNED_BLOCK_SIZE(req_size) \
    ARENA_ALIGN_SIZE(sizeof(block_header_t) + req_size)

#define BLOCK_BASE(ptr) ((void*) (((uint8_t*)ptr) - sizeof(block_header_t)))

#define BLOCK_SIZE(ptr) (((block_header_t*)BLOCK_BASE(ptr))->size)

#define IS_ALIGNED(ptr) (((uintptr_t) ptr) % ARENA_ALIGNMENT == 0)

#define IS_FREE(ptr) ((block_header_t*)ptr)->free

typedef struct {
    size_t free : 1;
    size_t size : sizeof(size_t) * 8 - 1;
    // Insert padding to future proof the implementation for new features
    char padding[ARENA_ALIGNMENT - sizeof(size_t)];
} block_header_t;

static_assert(
    sizeof(block_header_t) == ARENA_ALIGNMENT, 
    "Block header size must be 8 bytes"
);

typedef struct{
    alignas(max_align_t) arena_t arena;
    uint8_t data[STATIC_BUFFER_BYTES - sizeof(arena_t)];
} StaticBuffer;

static_assert(
    sizeof(StaticBuffer) <= STATIC_BUFFER_BYTES,
    "Static buffer objects violates requirements"
);

static_assert(
    STATIC_BUFFER_BYTES % ARENA_ALIGNMENT == 0,
    "Static arena size is not multiple of size alignment"
);

StaticBuffer temp_buffer = {
    .arena = {   
        .capacity = STATIC_BUFFER_BYTES,
        .position = sizeof(arena_t)
    },
    .data = {}
};

static arena_t* temp_arena = (arena_t*) &temp_buffer;

static void init_arena(arena_t* arena, size_t capacity) {
    arena->capacity = capacity;
    arena->position = sizeof(arena_t);
}

arena_t *arena_create(size_t capacity) {
    arena_t* arena = aligned_alloc(ARENA_ALIGNMENT  ,capacity);
    if (arena != NULL) {
        init_arena(arena, capacity);
    }
    return arena;
}

void arena_destroy(arena_t *arena) {
    free(arena);
}

static void* init_block(void* ptr, size_t size) {
    block_header_t* block = ptr;
    block->free = 0;
    block->size = size;
    return ((uint8_t*)block) + sizeof(block_header_t);
}

static bool arena_owns(arena_t * arena, void* ptr) {
    uint8_t* arena_b = (uint8_t*) arena;
    uint8_t* ptr_b = ptr;
    return (ptr_b >= arena_b) && (ptr_b < arena_b + arena->capacity);
}

static bool arena_last_alloc(arena_t* arena, block_header_t* block) {
    return ((uint8_t*)block) + block->size == ((uint8_t*)arena) + arena->position;
}


void* arena_alloc(arena_t *arena, size_t req_size) {
    size_t size = ALIGNED_BLOCK_SIZE(req_size);
    if (arena->position + size > arena->capacity) {
        return NULL;
    }
    void* allocation = ((uint8_t*)arena) + arena->position;
    arena->position += size;
    return init_block(allocation, size);
}

void* arena_calloc(arena_t *arena, size_t count, size_t size) {
    void* zone = arena_alloc(arena, count * size);
    if (zone != NULL) {
        memset(zone, 0, BLOCK_SIZE(zone) - sizeof(block_header_t));
    }
    return zone;
}

void* arena_realloc(arena_t *arena, void *ptr, size_t req_size) {
    if(ptr == NULL) {
        return arena_alloc(arena, req_size);
    }

    if (!IS_ALIGNED(BLOCK_BASE(ptr)) || !arena_owns(arena, ptr)) {
        return NULL;
    }

    size_t new_size = ALIGNED_BLOCK_SIZE(req_size);
    block_header_t* base = BLOCK_BASE(ptr);
    
    bool is_last = arena_last_alloc(arena, base);
    
    if (new_size <= base->size) {
        size_t diff = base->size - new_size;
        if (is_last) {
            arena->position -= diff;
        }
        base->size = new_size;
        return ptr;
    }

    if (is_last) {
        size_t diff = new_size - base->size;
        uintptr_t new_end = ((uintptr_t) base) + new_size;
        uintptr_t arena_end = ((uintptr_t)arena) + arena->capacity;
        // Use arena_owns here and make it also check block and
        if (new_end > arena_end){
            return NULL;
        }
        base->size += diff;
        arena->position += diff;
        return ptr;
    }

    void* new_ptr = arena_alloc(arena, req_size);
    if (new_ptr != NULL) {
        memmove(new_ptr, ptr, base->size - ARENA_ALIGNMENT);
    }
    return new_ptr;
}

void arena_free(arena_t *arena, void *ptr) {
    if (ptr == NULL || !arena_owns(arena, ptr)) {
        return;
    }

    block_header_t* base = BLOCK_BASE(ptr);

    if (base->free) {
        fprintf(stderr, "double free corruption in arena allocator: %p\n", ptr);
        return;
    }
    
    if (arena_last_alloc(arena, base)) {
        arena->position -= base->size;
    }
}

void arena_reset(arena_t *arena) {
    arena->position = sizeof(arena_t);
}

size_t arena_save(arena_t *arena) {
    return arena->position;
}

void arena_rewind(arena_t *arena, size_t checkpoint) {
    arena->position = checkpoint;
}

void* sbuff_alloc(size_t size) {
    return arena_alloc(temp_arena, size);
}

void* sbuff_calloc(size_t count, size_t size) {
    return arena_calloc(temp_arena, count, size);
}

void* sbuff_realloc(void *ptr, size_t size) {
    return arena_realloc(temp_arena, ptr, size);
}

void sbuff_free(void *ptr) {
    arena_free(temp_arena, ptr);
}

void sbuff_reset(void) {
    arena_reset(temp_arena);
}

size_t sbuff_save(void) {
    return arena_save(temp_arena);
}

void sbuff_rewind(size_t checkpoint) {
    arena_rewind(temp_arena, checkpoint);
}
