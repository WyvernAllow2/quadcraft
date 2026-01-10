#ifndef CHUNK_H
#define CHUNK_H

#include <stdint.h>

#include "block_type.h"
#include "math.h"
#include "math3d.h"

#define CHUNK_SIZE 32
#define CHUNK_VOLUME (CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE)

typedef struct Chunk {
    uint8_t blocks[CHUNK_VOLUME];
} Chunk;

Block_Type chunk_get_block_unsafe(const Chunk *chunk, iVec3 pos);
Block_Type chunk_get_block(const Chunk *chunk, iVec3 pos);

void chunk_set_block_unsafe(Chunk *chunk, iVec3 pos, Block_Type new_block);
void chunk_set_block(Chunk *chunk, iVec3 pos, Block_Type new_block);

bool chunk_is_block_transparent(const Chunk *chunk, iVec3 pos);

#endif /* CHUNK_H */
