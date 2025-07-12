#include "block_type.h"

/* clang-format off */
static const Block_Properties BLOCK_PROPERTY_TABLE[BLOCK_TYPE_COUNT] = {
    [BLOCK_AIR] = {
        .is_transparent = true,
    },

    [BLOCK_DIRT] = {
        .is_transparent = false,
    },

    [BLOCK_STONE] = {
        .is_transparent = false,
    },

    [BLOCK_GRASS] = {
        .is_transparent = false,
    }
};
/* clang-format on */

const Block_Properties *get_block_properties(Block_Type type) {
    return &BLOCK_PROPERTY_TABLE[type];
}
