#include "mesh_allocator.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "chunk.h"

#define MAX_VISIBLE_FACES ((CHUNK_VOLUME / 2) * 6)
#define MAX_INDICES (MAX_VISIBLE_FACES * 6)

static void init_buffers(Mesh_Allocator *allocator) {
    size_t vertex_buffer_size = sizeof(Block_Vertex) * allocator->vertex_capacity;
    size_t index_buffer_size = sizeof(uint32_t) * MAX_INDICES;

    uint32_t *indices = malloc(index_buffer_size);
    uint32_t offset = 0;
    for (uint32_t i = 0; i < MAX_INDICES; i += 6) {
        indices[i + 0] = offset + 3;
        indices[i + 1] = offset + 2;
        indices[i + 2] = offset + 1;
        indices[i + 3] = offset + 3;
        indices[i + 4] = offset + 1;
        indices[i + 5] = offset + 0;

        offset += 4;
    }

    glGenVertexArrays(1, &allocator->vao);
    glGenBuffers(1, &allocator->vbo);
    glGenBuffers(1, &allocator->ebo);

    glBindVertexArray(allocator->vao);
    glBindBuffer(GL_ARRAY_BUFFER, allocator->vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, allocator->ebo);

    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)vertex_buffer_size, NULL, GL_DYNAMIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)index_buffer_size, indices, GL_STATIC_DRAW);

    /* Position attribute */
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Block_Vertex),
                          (void *)offsetof(Block_Vertex, position));

    /* Normal attribute */
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Block_Vertex),
                          (void *)offsetof(Block_Vertex, normal));

    /* Layer attribute */
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Block_Vertex),
                          (void *)offsetof(Block_Vertex, layer));

    glBindVertexArray(0);
    free(indices);
}

static void free_buffers(Mesh_Allocator *allocator) {
    glDeleteBuffers(1, &allocator->ebo);
    glDeleteBuffers(1, &allocator->vbo);
    glDeleteVertexArrays(1, &allocator->vao);
}

void mesh_allocator_init(Mesh_Allocator *allocator, size_t vertex_capacity) {
    allocator->vertex_capacity = vertex_capacity;

    Free_List_Node *first_node = malloc(sizeof(Free_List_Node));
    first_node->start = 0;
    first_node->vertex_count = allocator->vertex_capacity;
    first_node->next = NULL;

    allocator->free_list = first_node;
    init_buffers(allocator);
}

void mesh_allocator_free(Mesh_Allocator *allocator) {
    free_buffers(allocator);

    Free_List_Node *node = allocator->free_list;
    while (node) {
        Free_List_Node *next = node->next;
        free(node);
        node = next;
    }
}

static Chunk_Mesh alloc_mesh(Mesh_Allocator *allocator, size_t vertex_count) {
    Free_List_Node *node = allocator->free_list;
    Free_List_Node *prev_node = NULL;

    while (node) {
        if (node->vertex_count >= vertex_count) {
            size_t start = node->start;

            if (node->vertex_count == vertex_count) {
                if (prev_node) {
                    prev_node->next = node->next;
                } else {
                    allocator->free_list = node->next;
                }
                free(node);
            } else {
                node->start += vertex_count;
                node->vertex_count -= vertex_count;
            }

            return (Chunk_Mesh){.start = start, .vertex_count = vertex_count};
        }

        prev_node = node;
        node = node->next;
    }

    fprintf(stderr, "Out of vertex buffer memory\n");
    exit(EXIT_FAILURE);
}

static void free_mesh(Mesh_Allocator *allocator, Chunk_Mesh *mesh) {
    Free_List_Node *node = allocator->free_list;
    Free_List_Node *prev = NULL;

    Free_List_Node *new_node = malloc(sizeof(Free_List_Node));
    new_node->start = mesh->start;
    new_node->vertex_count = mesh->vertex_count;
    new_node->next = NULL;

    /* Sort by start offset*/
    while (node && node->start < new_node->start) {
        prev = node;
        node = node->next;
    }

    new_node->next = node;
    if (prev) {
        prev->next = new_node;
    } else {
        allocator->free_list = new_node;
    }

    /* Merge with next */
    if (new_node->next && new_node->start + new_node->vertex_count == new_node->next->start) {
        Free_List_Node *next = new_node->next;
        new_node->vertex_count += next->vertex_count;
        new_node->next = next->next;
        free(next);
    }

    /* Merge with previous */
    if (prev && prev->start + prev->vertex_count == new_node->start) {
        prev->vertex_count += new_node->vertex_count;
        prev->next = new_node->next;
        free(new_node);
    }
}

void upload_mesh(Mesh_Allocator *allocator, Chunk_Mesh *mesh, const Block_Vertex *vertices,
                 size_t vertex_count) {
    if (mesh->vertex_count > 0) {
        free_mesh(allocator, mesh);
    }

    *mesh = alloc_mesh(allocator, vertex_count);

    glBindBuffer(GL_ARRAY_BUFFER, allocator->vbo);
    glBufferSubData(GL_ARRAY_BUFFER, mesh->start * sizeof(Block_Vertex),
                    vertex_count * sizeof(Block_Vertex), vertices);
}
