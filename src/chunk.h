#ifndef CHUNK_H
#define CHUNK_H

#include <stdint.h>

#include "block_type.h"
#include "math3d.h"

#define CHUNK_SIZE_X (16)
#define CHUNK_SIZE_Y (16)
#define CHUNK_SIZE_Z (16)
#define CHUNK_VOLUME (CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z)

typedef struct Chunk {
    uint8_t blocks[CHUNK_VOLUME];
} Chunk;

bool chunk_in_bounds(iVec3 local_pos);
size_t chunk_index(iVec3 local_pos);

Block_Type chunk_get_block(const Chunk *chunk, iVec3 local_pos);
void chunk_set_block(Chunk *chunk, iVec3 local_pos, Block_Type new_block);

#endif /* CHUNK_H */
