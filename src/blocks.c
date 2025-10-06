#include "blocks.h"

#include <assert.h>

/* clang-format off */
static const Block_Properties BLOCK_TABLE[BLOCK_TYPE_COUNT] = {
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
    },
};
/* clang-format on */

const Block_Properties *get_block_properties(Block_Type type) {
    assert(type >= 0 && type < BLOCK_TYPE_COUNT);
    return &BLOCK_TABLE[type];
}
