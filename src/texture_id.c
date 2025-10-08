#include "texture_id.h"

#include <assert.h>

/* clang-format off */
static const char *TEXTURE_FILENAME_TABLE[TEXTURE_ID_COUNT] = {
    [TEXTURE_DIRT] = "res/textures/dirt.png",
    [TEXTURE_GRASS_SIDE]= "res/textures/grass_side.png",
    [TEXTURE_GRASS]= "res/textures/grass.png",
    [TEXTURE_STONE]= "res/textures/stone.png",
    [TEXTURE_PLANK] = "res/textures/plank.png",
    [TEXTURE_BRICK] = "res/textures/brick.png",
    [TEXTURE_LOG_SIDE] = "res/textures/log_side.png",
    [TEXTURE_LOG_TOP] = "res/textures/log_top.png",
};
/* clang-format on */

const char *get_texture_filename(Texture_ID id) {
    assert(id >= 0 && id < TEXTURE_ID_COUNT);
    return TEXTURE_FILENAME_TABLE[id];
}
