#include "chunk.h"

bool chunk_in_bounds(iVec3 local_pos) {
    return local_pos.x >= 0 && local_pos.y >= 0 && local_pos.z >= 0 && local_pos.x < CHUNK_SIZE_X &&
           local_pos.y < CHUNK_SIZE_Y && local_pos.z < CHUNK_SIZE_Z;
}

size_t chunk_index(iVec3 local_pos) {
    size_t x = (size_t)local_pos.x;
    size_t y = (size_t)local_pos.y;
    size_t z = (size_t)local_pos.z;

    return x + y * CHUNK_SIZE_X + z * CHUNK_SIZE_X * CHUNK_SIZE_Y;
}

Block_Type chunk_get_block(const Chunk *chunk, iVec3 local_pos) {
    if (!chunk_in_bounds(local_pos)) {
        return BLOCK_AIR;
    }

    return chunk->blocks[chunk_index(local_pos)];
}

void chunk_set_block(Chunk *chunk, iVec3 local_pos, Block_Type new_block) {
    if (!chunk_in_bounds(local_pos)) {
        return;
    }

    Block_Type old_block = chunk_get_block(chunk, local_pos);
    if (old_block == new_block) {
        return;
    }

    chunk->blocks[chunk_index(local_pos)] = new_block;
    chunk->dirty = true;
}
