#include "blocks.h"

#include <assert.h>

#define FACES_UNIFORM(texture) {texture, texture, texture, texture, texture, texture}
#define FACES_SIDE_TOP_BOTTOM(side, top, bottom) {side, top, side, side, bottom, side}

/* clang-format off */
static const Block_Properties BLOCK_TABLE[BLOCK_TYPE_COUNT] = {
    [BLOCK_AIR] = {
        .is_transparent = true,
    },
    [BLOCK_DIRT] = {
        .is_transparent = false,
        .face_textures = FACES_UNIFORM(TEXTURE_DIRT),
    },
    [BLOCK_STONE] = {
        .is_transparent = false,
        .face_textures = FACES_UNIFORM(TEXTURE_STONE),
    },
    [BLOCK_GRASS] = {
        .is_transparent = false,
        .face_textures = FACES_SIDE_TOP_BOTTOM(TEXTURE_GRASS_SIDE, TEXTURE_GRASS, TEXTURE_DIRT),
    },
    [BLOCK_PLANK] = {
        .is_transparent = false,
        .face_textures = FACES_UNIFORM(TEXTURE_PLANK),
    },
    [BLOCK_BRICK] = {
        .is_transparent = false,
        .face_textures = FACES_UNIFORM(TEXTURE_BRICK),
    },
    [BLOCK_LOG] = {
        .is_transparent = false,
        .face_textures = FACES_SIDE_TOP_BOTTOM(TEXTURE_LOG_SIDE, TEXTURE_LOG_TOP, TEXTURE_LOG_TOP),
    }
};
/* clang-format on */

const Block_Properties *get_block_properties(Block_Type type) {
    assert(type >= 0 && type < BLOCK_TYPE_COUNT);
    return &BLOCK_TABLE[type];
}
