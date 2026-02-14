#ifndef __CLOX2_ARENA_H__
#define __CLOX2_ARENA_H__

#include <stddef.h>
#include <stdint.h>

#define ARENA_ALIGNMENT  (alignof(max_align_t))

typedef struct {
    alignas(max_align_t) size_t capacity;
    size_t position;
} arena_t;

static_assert(
    alignof(arena_t) == ARENA_ALIGNMENT &&
    sizeof(arena_t) == ARENA_ALIGNMENT &&
    sizeof(arena_t) % ARENA_ALIGNMENT == 0,
    "Arena object violates requirements"
);

#define ARENA_ALIGN_SIZE_IMPL(size, alignment) (((size) + alignment - 1) & ~(alignment - 1))

#define ARENA_ALIGN_SIZE(size) ARENA_ALIGN_SIZE_IMPL(size, ARENA_ALIGNMENT)

arena_t *arena_create(size_t capacity);

void arena_destroy(arena_t *arena);

void* arena_alloc(arena_t *arena, size_t size);

void* arena_calloc(arena_t *arena, size_t count, size_t size);

void* arena_realloc(arena_t *arena, void *ptr, size_t size);

void arena_free(arena_t *arena, void *ptr);

void arena_reset(arena_t *arena);

size_t arena_save(arena_t *arena);

void arena_rewind(arena_t *arena, size_t checkpoint);

void* sbuff_alloc(size_t size);

void* sbuff_calloc(size_t count, size_t size);

void* sbuff_realloc(void *ptr, size_t size);

void sbuff_free(void *ptr);

void sbuff_reset(void);

size_t sbuff_save(void);

void sbuff_rewind(size_t checkpoint);

#endif // __CLOX2_ARENA_H__
