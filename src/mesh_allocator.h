#ifndef MESH_ALLOCATOR_H
#define MESH_ALLOCATOR_H

#include <glad/gl.h>
#include <stdint.h>
#include <stdlib.h>

#include "math3d.h"

typedef struct Block_Vertex {
    Vec3 position;
    Vec3 normal;
    float layer;
} Block_Vertex;

typedef struct Chunk_Mesh {
    size_t start;
    size_t vertex_count;
} Chunk_Mesh;

typedef struct Free_List_Node Free_List_Node;

struct Free_List_Node {
    Free_List_Node *next;
    size_t start;
    size_t vertex_count;
};

typedef struct Mesh_Allocator {
    GLuint vao;
    GLuint vbo;
    GLuint ebo;

    size_t vertex_capacity;

    Free_List_Node *free_list;
} Mesh_Allocator;

void mesh_allocator_init(Mesh_Allocator *allocator, size_t vertex_capacity);
void mesh_allocator_free(Mesh_Allocator *allocator);

void upload_mesh(Mesh_Allocator *allocator, Chunk_Mesh *mesh, const Block_Vertex *vertices, size_t vertex_count);

#endif /* MESH_ALLOCATOR_H */
