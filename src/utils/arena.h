#ifndef ARENA_H
#define ARENA_H

#include <stdbool.h>
#include <stdint.h>
#include <stdalign.h>

#define KIB_TO_BYTES(x) ((uint64_t)(x) << 10)
#define MIB_TO_BYTES(x) ((uint64_t)(x) << 20)
#define GIB_TO_BYTES(x) ((uint64_t)(x) << 30)

#define ARENA_NEW(arena, T) \
    ((T *)arena_alloc_aligned((arena), sizeof(T), alignof(T)))

#define ARENA_NEW_ARRAY(arena, T, COUNT) \
    ((T *)arena_alloc_aligned((arena), sizeof(T) * (COUNT), alignof(T)))

typedef struct Arena {
    uint8_t *base;

    size_t reserved_size;
    size_t committed_size;
    size_t offset;

    size_t page_size;
} Arena;

bool arena_create(Arena *arena, size_t size);
void arena_destroy(Arena *arena);

void *arena_alloc_aligned(Arena *arena, size_t size, size_t align);
void *arena_alloc(Arena *arena, size_t size);
void arena_reset(Arena *arena);

#endif /* ARENA_H */
