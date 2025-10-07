#include "texture_id.h"

#include <assert.h>

/* clang-format off */
static const char *TEXTURE_FILENAME_TABLE[TEXTURE_ID_COUNT] = {
    [TEXTURE_DIRT] = "res/textures/dirt.png",
    [TEXTURE_GRASS_SIDE]= "res/textures/grass_side.png",
    [TEXTURE_GRASS]= "res/textures/grass.png",
    [TEXTURE_STONE]= "res/textures/stone.png",
    [TEXTURE_POSITIVE_X] = "res/textures/positive_x.png",
    [TEXTURE_POSITIVE_Y] = "res/textures/positive_y.png",
    [TEXTURE_POSITIVE_Z] = "res/textures/positive_z.png",
    [TEXTURE_NEGATIVE_X] = "res/textures/negative_x.png",
    [TEXTURE_NEGATIVE_Y] = "res/textures/negative_y.png",
    [TEXTURE_NEGATIVE_Z] = "res/textures/negative_z.png",
};
/* clang-format on */

const char *get_texture_filename(Texture_ID id) {
    assert(id >= 0 && id < TEXTURE_ID_COUNT);
    return TEXTURE_FILENAME_TABLE[id];
}
