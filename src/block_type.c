#include "block_type.h"

#include <assert.h>

#define TEXTURES_UNIFORM(tex) {tex, tex, tex, tex, tex, tex}
#define TEXTURES_SIDE_TOP_BOTTOM(side, top, bottom) {side, top, side, side, bottom, side}

/* clang-format off */
static const Block_Properties BLOCK_PROPERTY_TABLE[BLOCK_TYPE_COUNT] = {
    [BLOCK_AIR] = {
        .is_transparent = true,
    },
    [BLOCK_DIRT] = {
        .is_transparent = false,
        .textures = TEXTURES_UNIFORM(TEXTURE_ID_DIRT),
    },
};
/* clang-format on */

const Block_Properties *get_block_properties(Block_Type type) {
    assert(type >= 0 && type < BLOCK_TYPE_COUNT);
    return &BLOCK_PROPERTY_TABLE[type];
}
