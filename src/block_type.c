#include "block_type.h"

/* clang-format off */
static const Block_Properties BLOCK_PROPERTY_TABLE[BLOCK_TYPE_COUNT] = {
    [BLOCK_AIR] = {
        .is_transparent = true,
        .face_textures = {0}
    },

    [BLOCK_DIRT] = {
        .is_transparent = false,
        .face_textures = {
            [DIR_POS_X] = TEXTURE_DIRT,
            [DIR_POS_Y] = TEXTURE_DIRT,
            [DIR_POS_Z] = TEXTURE_DIRT,
            [DIR_NEG_X] = TEXTURE_DIRT,
            [DIR_NEG_Y] = TEXTURE_DIRT,
            [DIR_NEG_Z] = TEXTURE_DIRT,
        }
    },

    [BLOCK_STONE] = {
        .is_transparent = false,
        .face_textures = {
            [DIR_POS_X] = TEXTURE_STONE,
            [DIR_POS_Y] = TEXTURE_STONE,
            [DIR_POS_Z] = TEXTURE_STONE,
            [DIR_NEG_X] = TEXTURE_STONE,
            [DIR_NEG_Y] = TEXTURE_STONE,
            [DIR_NEG_Z] = TEXTURE_STONE,
        }
    },

    [BLOCK_GRASS] = {
        .is_transparent = false,
        .face_textures = {
            [DIR_POS_X] = TEXTURE_GRASS_SIDE,
            [DIR_POS_Y] = TEXTURE_GRASS_TOP,
            [DIR_POS_Z] = TEXTURE_GRASS_SIDE,
            [DIR_NEG_X] = TEXTURE_GRASS_SIDE,
            [DIR_NEG_Y] = TEXTURE_DIRT,
            [DIR_NEG_Z] = TEXTURE_GRASS_SIDE,
        }
    },

    [BLOCK_LOG] = {
        .is_transparent = false,
        .face_textures = {
            [DIR_POS_X] = TEXTURE_LOG_SIDE,
            [DIR_POS_Y] = TEXTURE_LOG_TOP,
            [DIR_POS_Z] = TEXTURE_LOG_SIDE,
            [DIR_NEG_X] = TEXTURE_LOG_SIDE,
            [DIR_NEG_Y] = TEXTURE_LOG_TOP,
            [DIR_NEG_Z] = TEXTURE_LOG_SIDE,
        }
    },

    [BLOCK_PLANK] = {
        .is_transparent = false,
        .face_textures = {
            [DIR_POS_X] = TEXTURE_PLANK,
            [DIR_POS_Y] = TEXTURE_PLANK,
            [DIR_POS_Z] = TEXTURE_PLANK,
            [DIR_NEG_X] = TEXTURE_PLANK,
            [DIR_NEG_Y] = TEXTURE_PLANK,
            [DIR_NEG_Z] = TEXTURE_PLANK,
        }
    },

    [BLOCK_BRICK] = {
        .is_transparent = false,
        .face_textures = {
            [DIR_POS_X] = TEXTURE_BRICK,
            [DIR_POS_Y] = TEXTURE_BRICK,
            [DIR_POS_Z] = TEXTURE_BRICK,
            [DIR_NEG_X] = TEXTURE_BRICK,
            [DIR_NEG_Y] = TEXTURE_BRICK,
            [DIR_NEG_Z] = TEXTURE_BRICK,
        }
    }
};
/* clang-format on */

const Block_Properties *get_block_properties(Block_Type type) {
    return &BLOCK_PROPERTY_TABLE[type];
}
