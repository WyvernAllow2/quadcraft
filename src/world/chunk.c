#include "chunk.h"

static size_t get_index(iVec3 pos) {
    return (size_t)(pos.x + CHUNK_SIZE * (pos.y + CHUNK_SIZE * pos.z));
}

Block_Type chunk_get_block_unsafe(const Chunk *chunk, iVec3 pos) {
    return chunk->blocks[get_index(pos)];
}

void chunk_set_block_unsafe(Chunk *chunk, iVec3 pos, Block_Type new_block) {
    chunk->blocks[get_index(pos)] = new_block;
}
