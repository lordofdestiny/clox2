#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <stdio.h>

#include "arena.h"

#define STATIC_BUFFER_BYTES 1024 * 1024 // 1MB

typedef struct {
    arena_t arena;
    uint8_t data[STATIC_BUFFER_BYTES - sizeof(arena_t)];
} StaticBuffer;

StaticBuffer temp_buffer = {
    .arena = {   
        .capacity = STATIC_BUFFER_BYTES,
        .position = sizeof(arena_t)
    },
    .data = {}
};

static arena_t* temp_arena = (arena_t*) &temp_buffer;

static arena_t* init_arena(arena_t* arena, size_t capacity) {
    arena->capacity = capacity;
    arena->position = sizeof(arena_t);

    return arena;
}

static bool arena_owns(arena_t * arena, void* ptr) {
    uint8_t* carena = (uint8_t*) arena;
    uint8_t* cptr = ptr;
    return (cptr >= carena) && (cptr < carena + arena->capacity);
}

static bool arena_last_alloc(arena_t* arena, void* base, size_t size) {
    return ((uint8_t*)base) + size == ((uint8_t*)arena) + arena->position;
}

arena_t *arena_create(size_t capacity) {
    arena_t* arena = calloc(capacity, sizeof(char));
    return init_arena(arena, capacity);
}

void arena_destroy(arena_t *arena) {
    free(arena);
}

void* arena_alloc(arena_t *arena, size_t req_size) {
    size_t size = ARENA_ALIGN_SIZE(req_size + ARENA_WORD_SIZE);
    if (arena->position + size > arena->capacity) return NULL;
    void* allocation = ((uint8_t*)arena) + arena->position;
    arena->position += size;
    *((uintptr_t*)allocation) = size;
    return ((uint8_t*)allocation) + ARENA_WORD_SIZE;
}

void* arena_calloc(arena_t *arena, size_t count, size_t size) {
    void* zone = arena_alloc(arena, count * size);
    if (zone != NULL) {
        size_t real_size = *(((uint8_t*)zone) - ARENA_WORD_SIZE);
        memset(zone, 0, real_size);
    }
    return zone;
}

void* arena_realloc(arena_t *arena, void *ptr, size_t req_size) {
    if(ptr == NULL) {
        return arena_alloc(arena, req_size);
    }

    if (!arena_owns(arena, ptr)) {
        return NULL;
    }

    size_t size = ARENA_ALIGN_SIZE(req_size + ARENA_WORD_SIZE);
    uint8_t* alloc_ptr =  ((uint8_t*) ptr) - ARENA_WORD_SIZE;
    size_t* size_ptr = (void*) alloc_ptr;
    
    bool is_last = arena_last_alloc(arena, alloc_ptr, *size_ptr);
    
    if (size <= *size_ptr) {
        size_t diff = *size_ptr - size;
        if (is_last) {
            arena->position -= diff;
        }
        *size_ptr = size;
        return ptr;
    }

    if (is_last) {
        size_t diff = size - *size_ptr;
        *size_ptr += diff;
        arena->position += diff;
        return ptr;
    }

    void* new_ptr = arena_alloc(arena, req_size);
    memmove(new_ptr, ptr, *size_ptr - sizeof(size_t));
    return new_ptr;
}

void arena_free(arena_t *arena, void *ptr) {
    if (ptr == NULL || !arena_owns(arena, ptr)) {
        return;
    }
    
    uint8_t* alloc_ptr =  ((uint8_t*) ptr) - ARENA_WORD_SIZE;
    size_t* size_ptr = (void*) alloc_ptr;

    if (arena_last_alloc(arena, alloc_ptr, *size_ptr)) {
        arena->position -= *size_ptr;
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
