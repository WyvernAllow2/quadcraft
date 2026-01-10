#include "chunk.h"

static bool in_bounds(iVec3 pos) {
    /* clang-format off */
    return pos.x >= 0 && 
           pos.y >= 0 && 
           pos.z >= 0 && 
           pos.x < CHUNK_SIZE &&
           pos.y < CHUNK_SIZE &&
           pos.z < CHUNK_SIZE;
    /* clang-format on */
}

static size_t get_index(iVec3 pos) {
    return (size_t)(pos.x + CHUNK_SIZE * (pos.y + CHUNK_SIZE * pos.z));
}

Block_Type chunk_get_block_unsafe(const Chunk *chunk, iVec3 pos) {
    return chunk->blocks[get_index(pos)];
}

Block_Type chunk_get_block(const Chunk *chunk, iVec3 pos) {
    if (!in_bounds(pos)) {
        return BLOCK_AIR;
    }

    return chunk_get_block_unsafe(chunk, pos);
}

void chunk_set_block_unsafe(Chunk *chunk, iVec3 pos, Block_Type new_block) {
    chunk->blocks[get_index(pos)] = new_block;
}

void chunk_set_block(Chunk *chunk, iVec3 pos, Block_Type new_block) {
    if (!in_bounds(pos)) {
        return;
    }

    chunk_set_block_unsafe(chunk, pos, new_block);
}

bool chunk_is_block_transparent(const Chunk *chunk, iVec3 pos) {
    Block_Type type = chunk_get_block(chunk, pos);
    return get_block_properties(type)->is_transparent;
}
