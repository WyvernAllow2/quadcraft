#ifndef MESH_ALLOCATOR_H
#define MESH_ALLOCATOR_H

#include <glad/gl.h>
#include <stdint.h>

#include "math3d.h"

typedef struct Vertex {
    Vec3 position;
    Vec3 normal;
    int texture;
} Vertex;

typedef struct Mesh {
    size_t offset;
    size_t length;
} Mesh;

typedef struct Mesh_Allocator {
    GLuint vao;
    GLuint vbo;
    GLuint ebo;

    size_t quad_capacity;

    Mesh *free;
    size_t free_count;
    size_t free_capacity;
} Mesh_Allocator;

void mesh_allocator_init(Mesh_Allocator *allocator, size_t quad_capacity);
void mesh_allocator_free(Mesh_Allocator *allocator);
void mesh_allocator_upload(Mesh_Allocator *allocator, Mesh *mesh, Vertex *vertices,
                           size_t vertex_count);

#endif /* MESH_ALLOCATOR_H */
