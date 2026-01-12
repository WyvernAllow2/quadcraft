#ifndef MESHING_H
#define MESHING_H

#include "utils/arena.h"
#include "world/chunk.h"

#define MESHING_DATA_SIZE (CHUNK_SIZE + 2)
#define MESHING_DATA_VOLUME (MESHING_DATA_SIZE * MESHING_DATA_SIZE * MESHING_DATA_SIZE)

typedef struct Meshing_Data {
    uint8_t blocks[MESHING_DATA_VOLUME];
} Meshing_Data;

uint32_t *mesh_chunk(const Meshing_Data *data, uint32_t *vertex_count, Arena *arena);
uint32_t *generate_index_buffer(uint32_t *index_count, Arena *arena);

#endif /* MESHING_H */
