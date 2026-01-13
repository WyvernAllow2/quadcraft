#include "world.h"

#include <assert.h>
#include <limits.h>

static bool in_world_bounds(iVec3 chunk_coord) {
    /* clang-format off */
    return chunk_coord.x >= 0 && 
           chunk_coord.y >= 0 && 
           chunk_coord.z >= 0 && 
           chunk_coord.x < WORLD_SIZE_X &&
           chunk_coord.y < WORLD_SIZE_Y &&
           chunk_coord.z < WORLD_SIZE_Z;
    /* clang-format on */
}

static size_t get_chunk_index(iVec3 chunk_coord) {
    return (size_t)(chunk_coord.x + WORLD_SIZE_X * (chunk_coord.y + WORLD_SIZE_Y * chunk_coord.z));
}

void world_push_dirty_chunk(World *world, Chunk *chunk) {
    assert(world != NULL);
    assert(chunk != NULL);

    /* We already added this chunk to the dirty list. */
    if (chunk->in_dirty_list) {
        return;
    }

    assert(world->dirty_list_count <= WORLD_VOLUME);

    world->dirty_list[world->dirty_list_count] = chunk;
    world->dirty_list_count++;

    chunk->in_dirty_list = true;
}

Chunk *world_pop_dirty_chunk(World *world, iVec3 player_position) {
    assert(world != NULL);

    if (world->dirty_list_count == 0) {
        return NULL;
    }

    int min_distance = INT_MAX;
    Chunk *closest_dirty = NULL;
    size_t closest_dirty_index = 0;

    for (size_t i = 0; i < world->dirty_list_count; i++) {
        Chunk *chunk = world->dirty_list[i];
        assert(chunk != NULL);

        iVec3 delta = ivec3_sub(player_position, chunk->coord);
        float distance = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;

        if (distance < min_distance) {
            min_distance = distance;
            closest_dirty = chunk;
            closest_dirty_index = i;
        }
    }

    if (!closest_dirty) {
        return NULL;
    }

    /* Swap and pop */
    if (world->dirty_list_count > 1) {
        world->dirty_list[closest_dirty_index] = world->dirty_list[world->dirty_list_count - 1];
    }

    world->dirty_list_count--;

    closest_dirty->in_dirty_list = false;
    return closest_dirty;
}

Chunk *world_get_chunk(World *world, iVec3 chunk_coord) {
    assert(world != NULL);

    if (!in_world_bounds(chunk_coord)) {
        return NULL;
    }

    return &world->chunk[get_chunk_index(chunk_coord)];
}

Block_Type world_get_block(const World *world, iVec3 position) {
    assert(world != NULL);

    iVec3 chunk_coord = ivec3_floor_div(position, CHUNK_SIZE);
    if (!in_world_bounds(chunk_coord)) {
        return BLOCK_DIRT;
    }

    const Chunk *chunk = &world->chunk[get_chunk_index(chunk_coord)];

    iVec3 block_coord = ivec3_mod(position, CHUNK_SIZE);
    return chunk_get_block_unsafe(chunk, block_coord);
}

void world_set_block(World *world, iVec3 position, Block_Type new_block) {
    assert(world != NULL);

    iVec3 chunk_coord = ivec3_floor_div(position, CHUNK_SIZE);
    Chunk *chunk = world_get_chunk(world, chunk_coord);
    if (!chunk) {
        return;
    }

    iVec3 block_coord = ivec3_mod(position, CHUNK_SIZE);
    chunk_set_block_unsafe(chunk, block_coord, new_block);

    world_push_dirty_chunk(world, chunk);

    /* If a block was set on the edge of this chunk, then we also need to mark the affected
     * neighboring chunks as dirty, as their meshes are now invalid. */
    bool affected_neighbors[DIRECTION_COUNT] = {
        [DIR_POSITIVE_X] = block_coord.x == CHUNK_SIZE - 1,
        [DIR_POSITIVE_Y] = block_coord.y == CHUNK_SIZE - 1,
        [DIR_POSITIVE_Z] = block_coord.z == CHUNK_SIZE - 1,
        [DIR_NEGATIVE_X] = block_coord.x == 0,
        [DIR_NEGATIVE_Y] = block_coord.y == 0,
        [DIR_NEGATIVE_Z] = block_coord.z == 0,
    };

    for (Direction dir = 0; dir < DIRECTION_COUNT; dir++) {
        if (!affected_neighbors[dir]) {
            continue;
        }

        iVec3 neighbor_chunk_coord = ivec3_add(chunk_coord, direction_to_ivec3(dir));
        Chunk *neighbor = world_get_chunk(world, neighbor_chunk_coord);
        if (neighbor != NULL) {
            world_push_dirty_chunk(world, neighbor);
        }
    }
}
