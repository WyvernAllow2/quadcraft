#ifndef RANGE_ALLOCATOR_H
#define RANGE_ALLOCATOR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define NODE_POOL_SIZE 512

typedef struct Range {
    size_t start;
    size_t size;
} Range;

typedef struct Node {
    Range range;
    struct Node *next;
} Node;

typedef struct Range_Allocator {
    Node *free_list_head;
    size_t capacity;
    size_t used;

    Node *pool_head;
    Node node_pool[NODE_POOL_SIZE];
} Range_Allocator;

void range_allocator_create(Range_Allocator *Range_Allocator, size_t capacity);
void range_allocator_destroy(Range_Allocator *Range_Allocator);

Range range_alloc(Range_Allocator *Range_Allocator, size_t size);
void range_free(Range_Allocator *Range_Allocator, Range range);

#endif /* RANGE_ALLOCATOR_H */
