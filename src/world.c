#include "world.h"

#include <assert.h>

void world_mark_chunk_dirty(World *world, Chunk *chunk) {
    if (chunk->is_dirty) {
        return;
    }

    chunk->is_dirty = true;

    if (world->dirty_chunk_count + 1 > WORLD_VOLUME) {
        assert(false);
    }

    world->dirty_chunks[world->dirty_chunk_count] = chunk;
    world->dirty_chunk_count++;
}

static size_t get_chunk_index(iVec3 chunk_position) {
    size_t x = (size_t)chunk_position.x;
    size_t y = (size_t)chunk_position.y;
    size_t z = (size_t)chunk_position.z;
    return x + WORLD_SIZE_X * y + (WORLD_SIZE_X * WORLD_SIZE_Y) * z;
}

static bool is_chunk_in_world_bounds(iVec3 chunk_position) {
    return chunk_position.x >= 0 && chunk_position.y >= 0 && chunk_position.z >= 0 &&
           chunk_position.x < WORLD_SIZE_X && chunk_position.y < WORLD_SIZE_Y &&
           chunk_position.z < WORLD_SIZE_Z;
}

static bool is_on_chunk_edge(iVec3 position) {
    int max = CHUNK_SIZE - 1;

    return position.x == 0 || position.y == 0 || position.z == 0 || position.x == max ||
           position.y == max || position.z == max;
}

size_t get_block_index(iVec3 local_position) {
    size_t x = (size_t)local_position.x;
    size_t y = (size_t)local_position.y;
    size_t z = (size_t)local_position.z;
    return x + CHUNK_SIZE * (y + CHUNK_SIZE * z);
}

Block_Type chunk_get_block_unsafe(const Chunk *chunk, iVec3 local_position) {
    return chunk->blocks[get_block_index(local_position)];
}

void chunk_set_block_unsafe(Chunk *chunk, iVec3 local_position, Block_Type type) {
    chunk->blocks[get_block_index(local_position)] = type;
}

Chunk *world_get_chunk_unsafe(World *world, iVec3 chunk_position) {
    return &world->chunk[get_chunk_index(chunk_position)];
}

Chunk *world_get_chunk(World *world, iVec3 chunk_position) {
    if (!is_chunk_in_world_bounds(chunk_position)) {
        return NULL;
    }

    return world_get_chunk_unsafe(world, chunk_position);
}

Block_Type world_get_block_unsafe(World *world, iVec3 block_position) {
    iVec3 chunk_position = {
        block_position.x / CHUNK_SIZE,
        block_position.y / CHUNK_SIZE,
        block_position.z / CHUNK_SIZE,
    };

    iVec3 local_position = {
        mod(block_position.x, CHUNK_SIZE),
        mod(block_position.y, CHUNK_SIZE),
        mod(block_position.z, CHUNK_SIZE),
    };

    Chunk *chunk = world_get_chunk_unsafe(world, chunk_position);
    return chunk->blocks[get_block_index(local_position)];
}

Block_Type world_get_block(World *world, iVec3 block_position) {
    iVec3 chunk_position = {
        block_position.x / CHUNK_SIZE,
        block_position.y / CHUNK_SIZE,
        block_position.z / CHUNK_SIZE,
    };

    if (!is_chunk_in_world_bounds(chunk_position)) {
        return BLOCK_AIR;
    }

    iVec3 local_position = {
        mod(block_position.x, CHUNK_SIZE),
        mod(block_position.y, CHUNK_SIZE),
        mod(block_position.z, CHUNK_SIZE),
    };

    Chunk *chunk = world_get_chunk_unsafe(world, chunk_position);
    return chunk->blocks[get_block_index(local_position)];
}

void world_set_block_unsafe(World *world, iVec3 block_position, Block_Type type) {
    iVec3 chunk_position = {
        block_position.x / CHUNK_SIZE,
        block_position.y / CHUNK_SIZE,
        block_position.z / CHUNK_SIZE,
    };

    iVec3 local_position = {
        mod(block_position.x, CHUNK_SIZE),
        mod(block_position.y, CHUNK_SIZE),
        mod(block_position.z, CHUNK_SIZE),
    };

    Chunk *chunk = world_get_chunk_unsafe(world, chunk_position);
    chunk->blocks[get_block_index(local_position)] = type;
}

void world_set_block(World *world, iVec3 block_position, Block_Type type) {
    iVec3 chunk_position = {
        block_position.x / CHUNK_SIZE,
        block_position.y / CHUNK_SIZE,
        block_position.z / CHUNK_SIZE,
    };

    if (!is_chunk_in_world_bounds(chunk_position)) {
        return;
    }

    iVec3 local_position = {
        mod(block_position.x, CHUNK_SIZE),
        mod(block_position.y, CHUNK_SIZE),
        mod(block_position.z, CHUNK_SIZE),
    };

    Chunk *chunk = world_get_chunk_unsafe(world, chunk_position);

    Block_Type old_type = chunk->blocks[get_block_index(local_position)];
    if (old_type == type) {
        return;
    }

    chunk->blocks[get_block_index(local_position)] = type;
    world_mark_chunk_dirty(world, chunk);

    if (is_on_chunk_edge(local_position)) {
        for (Direction d = 0; d < DIRECTION_COUNT; d++) {
            iVec3 neighbor_pos = ivec3_add(chunk_position, direction_to_ivec3(d));
            Chunk *neighbor = world_get_chunk(world, neighbor_pos);
            if (neighbor) {
                world_mark_chunk_dirty(world, neighbor);
            }
        }
    }
}

Hit_Result world_raycast(World *world, Vec3 origin, Vec3 direction) {
    iVec3 map = {
        floorf(origin.x),
        floorf(origin.y),
        floorf(origin.z),
    };

    Vec3 delta_dist = {
        .x = fabsf(1.0f / direction.x),
        .y = fabsf(1.0f / direction.y),
        .z = fabsf(1.0f / direction.z),
    };

    Vec3 step_dir = {
        .x = signf(direction.x),
        .y = signf(direction.y),
        .z = signf(direction.z),
    };

    Vec3 side_dist = {
        .x = (direction.x > 0) ? ((map.x + 1.0f - origin.x) * delta_dist.x)
                               : ((origin.x - map.x) * delta_dist.x),
        .y = (direction.y > 0) ? ((map.y + 1.0f - origin.y) * delta_dist.y)
                               : ((origin.y - map.y) * delta_dist.y),
        .z = (direction.z > 0) ? ((map.z + 1.0f - origin.z) * delta_dist.z)
                               : ((origin.z - map.z) * delta_dist.z),
    };

    Direction dir;

    for (int i = 0; i < 1000; i++) {
        if (side_dist.x < side_dist.y && side_dist.x < side_dist.z) {
            map.x += step_dir.x;
            side_dist.x += delta_dist.x;
            dir = step_dir.x < 0 ? DIR_POSITIVE_X : DIR_NEGATIVE_X;
        } else if (side_dist.y < side_dist.z) {
            map.y += step_dir.y;
            side_dist.y += delta_dist.y;
            dir = step_dir.y < 0 ? DIR_POSITIVE_Y : DIR_NEGATIVE_Y;
        } else {
            map.z += step_dir.z;
            side_dist.z += delta_dist.z;
            dir = step_dir.z < 0 ? DIR_POSITIVE_Z : DIR_NEGATIVE_Z;
        }

        if (world_get_block(world, map) != BLOCK_AIR) {
            Hit_Result result = {
                .did_hit = true,
                .position = {(float)map.x, (float)map.y, (float)map.z},
                .normal = direction_to_ivec3(dir),
            };
            return result;
        }
    }

    Hit_Result result = {.did_hit = false};
    return result;
}
