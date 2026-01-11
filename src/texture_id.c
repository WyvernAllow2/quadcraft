#include "texture_id.h"

#include <assert.h>

static const char *TEXTURE_FILENAMES[TEXTURE_ID_COUNT] = {
    [TEXTURE_ID_DIRT] = "res/textures/dirt.png",
};

const char *get_texture_filename(Texture_ID id) {
    assert(id >= 0 && id < TEXTURE_ID_COUNT);
    return TEXTURE_FILENAMES[id];
}
