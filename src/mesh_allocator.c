#include "mesh_allocator.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHUNK_VOLUME (32 * 32 * 32)

/* The maximum number of quads a chunk could possibly have. Assuming the worse-case scenario of a 3D
   checkerboard pattern, then half the blocks would have all 6 faces exposed.
*/
#define MAX_QUADS ((CHUNK_VOLUME / 2) * 6)
#define MAX_VERTS (MAX_QUADS * 4)
#define MAX_INDICES (MAX_QUADS * 6)

static void insert_free_mesh(Mesh_Allocator *allocator, size_t index, Mesh mesh) {
    if (index > allocator->free_count) {
        index = allocator->free_count;
    }

    if (allocator->free_count >= allocator->free_capacity) {
        size_t new_capacity = allocator->free_capacity == 0 ? 8 : allocator->free_capacity * 1.5;
        Mesh *new_array = realloc(allocator->free, new_capacity * sizeof(Mesh));
        if (!new_array) {
            perror("realloc() failed");
            exit(EXIT_FAILURE);
        }
        allocator->free = new_array;
        allocator->free_capacity = new_capacity;
    }

    memmove(&allocator->free[index + 1], &allocator->free[index],
            (allocator->free_count - index) * sizeof(Mesh));

    allocator->free[index] = mesh;
    allocator->free_count++;
}

static void remove_free_mesh(Mesh_Allocator *allocator, size_t index) {
    if (index == allocator->free_count - 1) {
        allocator->free_count--;
        return;
    }

    memmove(&allocator->free[index], &allocator->free[index + 1],
            (allocator->free_count - index - 1) * sizeof(Mesh));
    allocator->free_count--;
}

static bool are_blocks_adjacent(const Mesh *left, const Mesh *right) {
    assert(left->offset < right->offset);
    return left->offset + left->length == right->offset;
}

static void deallocate_mesh(Mesh_Allocator *allocator, Mesh mesh) {
    size_t index = 0;
    while (index < allocator->free_count && allocator->free[index].offset < mesh.offset) {
        index++;
    }

    insert_free_mesh(allocator, index, mesh);

    /* Ensure this is not the first block */
    if (index > 0) {
        Mesh *prev = &allocator->free[index - 1];
        Mesh *curr = &allocator->free[index];

        if (are_blocks_adjacent(prev, curr)) {
            /* Expand the previous block to the size of the current block, and delete the current
             * block.*/
            prev->length += curr->length;
            remove_free_mesh(allocator, index);
            index--;
        }
    }

    /* Ensure this is not the last block */
    if (index + 1 < allocator->free_count) {
        Mesh *curr = &allocator->free[index];
        Mesh *next = &allocator->free[index + 1];

        if (are_blocks_adjacent(curr, next)) {
            /* Expand the current block to the size of the next block, and delete the next block. */
            curr->length += next->length;
            remove_free_mesh(allocator, index + 1);
        }
    }
}

static Mesh allocate_mesh(Mesh_Allocator *allocator, size_t length) {
    for (size_t i = 0; i < allocator->free_count; i++) {
        Mesh *free = &allocator->free[i];

        if (free->length >= length) {
            Mesh mesh = {
                .offset = free->offset,
                .length = length,
            };

            free->offset += length;
            free->length -= length;

            if (free->length == 0) {
                remove_free_mesh(allocator, i);
            }

            return mesh;
        }
    }

    return (Mesh){0, 0};
}

void mesh_allocator_init(Mesh_Allocator *allocator, size_t quad_capacity) {
    assert(allocator != NULL);
    assert(quad_capacity != 0);

    *allocator = (Mesh_Allocator){
        .quad_capacity = quad_capacity,
    };

    size_t vertex_buffer_size = allocator->quad_capacity * 4 * sizeof(Vertex);
    size_t index_buffer_size = MAX_INDICES * sizeof(uint32_t);

    glGenVertexArrays(1, &allocator->vao);
    glGenBuffers(1, &allocator->vbo);
    glGenBuffers(1, &allocator->ebo);

    glBindVertexArray(allocator->vao);
    glBindBuffer(GL_ARRAY_BUFFER, allocator->vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, allocator->ebo);

    uint32_t *indices = malloc(index_buffer_size);
    for (uint32_t i = 0; i < MAX_QUADS; i++) {
        indices[i * 6 + 0] = 0 + i * 4;
        indices[i * 6 + 1] = 1 + i * 4;
        indices[i * 6 + 2] = 3 + i * 4;
        indices[i * 6 + 3] = 1 + i * 4;
        indices[i * 6 + 4] = 2 + i * 4;
        indices[i * 6 + 5] = 3 + i * 4;
    }

    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)vertex_buffer_size, NULL, GL_DYNAMIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)index_buffer_size, indices, GL_STATIC_DRAW);

    free(indices);

    /* Position attribute */
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (const void *)offsetof(Vertex, position));

    /* Normal attribute */
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (const void *)offsetof(Vertex, normal));

    glBindVertexArray(0);

    insert_free_mesh(allocator, 0, (Mesh){0, allocator->quad_capacity * 4});
}

void mesh_allocator_free(Mesh_Allocator *allocator) {
    assert(allocator != NULL);

    glDeleteBuffers(1, &allocator->ebo);
    glDeleteBuffers(1, &allocator->vbo);
    glDeleteVertexArrays(1, &allocator->vao);

    free(allocator->free);
}

static void upload_mesh(Mesh_Allocator *allocator, Mesh mesh, Vertex *vertices) {
    GLsizeiptr nbytes_vertex_data = (GLsizeiptr)(sizeof(Vertex) * mesh.length);
    GLsizeiptr nbytes_offset = (GLsizeiptr)(sizeof(Vertex) * mesh.offset);

    glBindBuffer(GL_ARRAY_BUFFER, allocator->vbo);
    glBufferSubData(GL_ARRAY_BUFFER, nbytes_offset, nbytes_vertex_data, vertices);

    fprintf(stderr, "Uploaded %zu KiB of vertex data\n", (size_t)nbytes_vertex_data / 1024);
}

void mesh_allocator_upload(Mesh_Allocator *allocator, Mesh *mesh, Vertex *vertices,
                           size_t vertex_count) {
    assert(allocator != NULL);
    assert(mesh != NULL);

    if (mesh->length == vertex_count) {
        fprintf(stderr, "Mesh size did not change. Uploading without reallocation\n");
        upload_mesh(allocator, *mesh, vertices);
        return;
    }

    if (mesh->length != 0) {
        fprintf(stderr, "Deallocating old mesh: {%zu, %zu}\n", mesh->offset, mesh->length);
        deallocate_mesh(allocator, *mesh);
    }

    Mesh new_mesh = allocate_mesh(allocator, vertex_count);
    if (new_mesh.length != 0) {
        *mesh = new_mesh;
        fprintf(stderr, "Allocated new mesh: {%zu, %zu}\n", mesh->offset, mesh->length);
        upload_mesh(allocator, *mesh, vertices);
    }
}
