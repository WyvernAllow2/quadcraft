#ifndef MESHING_H
#define MESHING_H

#include "arena.h"
#include "chunk.h"

uint32_t *mesh_chunk(const Chunk *chunk, uint32_t *vertex_count, Arena *arena);
uint32_t *generate_index_buffer(uint32_t *index_count, Arena *arena);

#endif /* MESHING_H */
