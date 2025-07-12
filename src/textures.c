#include "textures.h"

static const char *TEXTURE_FILENAME_TABLE[TEXTURE_ID_COUNT] = {
    [TEXTURE_DIRT] = "res/textures/dirt.png",
    [TEXTURE_STONE] = "res/textures/stone.png",
    [TEXTURE_GRASS_SIDE] = "res/textures/grass_side.png",
    [TEXTURE_GRASS_TOP] = "res/textures/grass_top.png",
    [TEXTURE_LOG_SIDE] = "res/textures/log_side.png",
    [TEXTURE_LOG_TOP] = "res/textures/log_top.png",
    [TEXTURE_PLANK] = "res/textures/plank.png",
    [TEXTURE_BRICK] = "res/textures/brick.png",
};

const char *get_texture_path(Texture_ID id) {
    return TEXTURE_FILENAME_TABLE[id];
}
