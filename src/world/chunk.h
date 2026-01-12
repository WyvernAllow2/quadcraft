#ifndef CHUNK_H
#define CHUNK_H

#include <stdint.h>

#include "utils/math3d.h"
#include "utils/range_allocator.h"
#include "world/block_type.h"

#define CHUNK_SIZE 32
#define CHUNK_VOLUME (CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE)

typedef struct Chunk {
    iVec3 coord;

    uint8_t blocks[CHUNK_VOLUME];
    bool in_dirty_list;

    Range mesh;
} Chunk;

Block_Type chunk_get_block_unsafe(const Chunk *chunk, iVec3 pos);
void chunk_set_block_unsafe(Chunk *chunk, iVec3 pos, Block_Type new_block);

#endif /* CHUNK_H */
