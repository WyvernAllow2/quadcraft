#ifndef WORLD_H
#define WORLD_H

#include <stdbool.h>
#include <stdint.h>

#include "blocks.h"
#include "mesh_allocator.h"

#define CHUNK_SIZE (32)
#define CHUNK_VOLUME (CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE)

typedef struct Chunk {
    uint8_t blocks[CHUNK_VOLUME];
    bool is_dirty;
    iVec3 coord;
    Mesh mesh;
} Chunk;

size_t get_block_index(iVec3 local_position);
Block_Type chunk_get_block_unsafe(const Chunk *chunk, iVec3 local_position);
void chunk_set_block_unsafe(Chunk *chunk, iVec3 local_position, Block_Type type);

#define WORLD_SIZE_X (16)
#define WORLD_SIZE_Y (10)
#define WORLD_SIZE_Z (16)
#define WORLD_VOLUME (WORLD_SIZE_X * WORLD_SIZE_Y * WORLD_SIZE_Z)

typedef struct World {
    Chunk chunk[WORLD_VOLUME];

    Chunk *dirty_chunks[WORLD_VOLUME];
    size_t dirty_chunk_count;
} World;

void world_mark_chunk_dirty(World *world, Chunk *chunk);
Chunk *world_get_chunk_unsafe(World *world, iVec3 chunk_position);
Chunk *world_get_chunk(World *world, iVec3 chunk_position);

Block_Type world_get_block_unsafe(World *world, iVec3 block_position);
Block_Type world_get_block(World *world, iVec3 block_position);

void world_set_block_unsafe(World *world, iVec3 block_position, Block_Type type);
void world_set_block(World *world, iVec3 block_position, Block_Type type);

typedef struct Hit_Result {
    bool did_hit;
    iVec3 position;
    iVec3 normal;
    float t;
} Hit_Result;

Hit_Result world_raycast(World  *world, Vec3 origin, Vec3 direction);

#endif /* WORLD_H */
