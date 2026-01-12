#ifndef WORLD_H
#define WORLD_H

#include "chunk.h"

#define WORLD_SIZE_X (32)
#define WORLD_SIZE_Y (8)
#define WORLD_SIZE_Z (32)
#define WORLD_VOLUME (WORLD_SIZE_X * WORLD_SIZE_Y * WORLD_SIZE_Z)

typedef struct World {
    Chunk chunk[WORLD_VOLUME];

    Chunk *dirty_list[WORLD_VOLUME];
    size_t dirty_list_count;
} World;

Chunk *world_get_chunk(World *world, iVec3 chunk_coord);

void world_push_dirty_chunk(World *world, Chunk *chunk);
Chunk *world_pop_dirty_chunk(World *world, iVec3 player_position);

Block_Type world_get_block(const World *world, iVec3 position);
void world_set_block(World *world, iVec3 position, Block_Type new_block);

#endif /* WORLD_H */
