#include "range_allocator.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static bool are_ranges_adjacent(const Range *a, const Range *b) {
    assert(a != NULL && b != NULL);
    assert(a->start < b->start);
    return a->start + a->size == b->start;
}

static Node *alloc_node(Range_Allocator *allocator, Range range) {
    Node *node = allocator->pool_head;
    if (!node) {
        fprintf(stderr, "Range allocator is out of pool memory");
        exit(EXIT_FAILURE);
    }

    allocator->pool_head = allocator->pool_head->next;

    *node = (Node){
        .range = range,
        .next = NULL,
    };

    return node;
}

static void free_node(Range_Allocator *allocator, Node *node) {
    node->next = allocator->pool_head;
    allocator->pool_head = node;
}

static void free_list_insert(Range_Allocator *allocator, Node *prev, Node *new) {
    if (!prev) {
        new->next = allocator->free_list_head;
        allocator->free_list_head = new;
    } else {
        new->next = prev->next;
        prev->next = new;
    }
}

static void free_list_remove(Range_Allocator *allocator, Node *prev, Node *del) {
    if (!prev) {
        allocator->free_list_head = del->next;
    } else {
        prev->next = del->next;
    }

    free_node(allocator, del);
}

void range_allocator_create(Range_Allocator *allocator, size_t capacity) {
    assert(allocator != NULL);
    *allocator = (Range_Allocator){
        .capacity = capacity,
    };

    for (size_t i = 0; i < NODE_POOL_SIZE; i++) {
        Node *node = &allocator->node_pool[i];
        free_node(allocator, node);
    }

    allocator->free_list_head = alloc_node(allocator, (Range){0, capacity});
}

void range_allocator_destroy(Range_Allocator *allocator) {
    assert(allocator != NULL);

    Node *node = allocator->free_list_head;
    while (node) {
        Node *next = node->next;
        free_node(allocator, node);
        node = next;
    }
    allocator->free_list_head = NULL;
}

Range range_alloc(Range_Allocator *allocator, size_t size) {
    assert(allocator != NULL);
    assert(size > 0);

    Node *node = allocator->free_list_head;
    Node *prev = NULL;

    while (node) {
        if (node->range.size >= size) {
            Range range = {node->range.start, size};
            node->range.start += size;
            node->range.size -= size;

            if (node->range.size == 0) {
                free_list_remove(allocator, prev, node);
            }

            allocator->used += size;
            return range;
        }
        prev = node;
        node = node->next;
    }

    fprintf(stderr, "Out of free ranges");
    exit(EXIT_FAILURE);
}

void range_free(Range_Allocator *allocator, Range range) {
    assert(allocator != NULL);
    assert(range.start + range.size <= allocator->capacity);

    Node *node = allocator->free_list_head;
    Node *prev = NULL;

    while (node && node->range.start < range.start) {
        prev = node;
        node = node->next;
    }

    Node *new = alloc_node(allocator, range);
    free_list_insert(allocator, prev, new);

    if (prev && are_ranges_adjacent(&prev->range, &new->range)) {
        prev->range.size += new->range.size;
        prev->next = new->next;
        free_node(allocator, new);
        new = prev;
    }

    if (new->next && are_ranges_adjacent(&new->range, &new->next->range)) {
        Node *next = new->next;
        new->range.size += next->range.size;
        new->next = next->next;
        free_node(allocator, next);
    }

    allocator->used -= range.size;
}
