#include "arena.h"

#include <assert.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

static size_t get_page_size(void) {
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return info.dwPageSize;
}

static void *reserve_memory(size_t size) {
    return VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_NOACCESS);
}

static bool commit_memory(void *addr, size_t size) {
    return VirtualAlloc(addr, size, MEM_COMMIT, PAGE_READWRITE) != NULL;
}

static void free_reserved_memory(void *addr, size_t size) {
    (void)size;
    VirtualFree(addr, 0, MEM_RELEASE);
}

#else
#include <sys/mman.h>
#include <unistd.h>

static size_t get_page_size(void) {
    return (size_t)sysconf(_SC_PAGESIZE);
}

static void *reserve_memory(size_t size) {
    void *ptr = mmap(NULL, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) {
        return NULL;
    }

    return ptr;
}

static bool commit_memory(void *addr, size_t size) {
    return mprotect(addr, size, PROT_READ | PROT_WRITE) == 0;
}

static void free_reserved_memory(void *addr, size_t size) {
    munmap(addr, size);
}

#endif

static bool is_power_of_two(uintptr_t align) {
    return align != 0 && (align & (align - 1)) == 0;
}

static uintptr_t align_forward(uintptr_t ptr, size_t align) {
    assert(is_power_of_two(align));
    return (ptr + align - 1) & ~(align - 1);
}

bool arena_create(Arena *arena, size_t size) {
    assert(arena != NULL);
    assert(size > 0);

    *arena = (Arena){0};

    arena->page_size = get_page_size();
    arena->reserved_size = align_forward(size, arena->page_size);
    arena->base = reserve_memory(arena->reserved_size);
    if (!arena->base) {
        return false;
    }

    return true;
}

void arena_destroy(Arena *arena) {
    if (!arena || !arena->base) {
        return;
    }

    free_reserved_memory(arena->base, arena->reserved_size);
    *arena = (Arena){0};
}

void *arena_alloc_aligned(Arena *arena, size_t size, size_t align) {
    assert(arena != NULL);
    if (size == 0) {
        return NULL;
    }

    /* Align the arena offset pointer to the requested alignment */
    uintptr_t curr_ptr = (uintptr_t)arena->base + (uintptr_t)arena->offset;
    uintptr_t offset = align_forward(curr_ptr, align) - (uintptr_t)arena->base;

    uintptr_t new_offset = offset + size;

    if (new_offset > arena->reserved_size) {
        fprintf(stderr, "Arena is out of reserved memory\n");
        exit(EXIT_FAILURE);
    }

    if (new_offset > arena->committed_size) {
        /* Align the commit to the next page boundary. */
        uintptr_t new_commit_ptr = align_forward(new_offset, arena->page_size);
        if (new_commit_ptr > arena->reserved_size) {
            new_commit_ptr = arena->reserved_size;
        }

        size_t size_to_commit = (size_t)(new_commit_ptr - arena->committed_size);
        uint8_t *commit_addr = arena->base + arena->committed_size;

        if (!commit_memory(commit_addr, size_to_commit)) {
            fprintf(stderr, "Failed to commit memory\n");
            exit(EXIT_FAILURE);
        }

        arena->committed_size = new_commit_ptr;
    }

    uint8_t *ptr = arena->base + offset;
    arena->offset = new_offset;
    return memset(ptr, 0, size);
}

void *arena_alloc(Arena *arena, size_t size) {
    return arena_alloc_aligned(arena, size, alignof(max_align_t));
}

void arena_reset(Arena *arena) {
    assert(arena);
    arena->offset = 0;
}
