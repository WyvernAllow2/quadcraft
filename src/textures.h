#ifndef TEXTURES_H
#define TEXTURES_H

#define TEXTURE_WIDTH (16)
#define TEXTURE_HEIGHT (16)

typedef enum Texture_ID {
    TEXTURE_DIRT,
    TEXTURE_STONE,
    TEXTURE_GRASS_SIDE,
    TEXTURE_GRASS_TOP,
    TEXTURE_LOG_SIDE,
    TEXTURE_LOG_TOP,
    TEXTURE_PLANK,
    TEXTURE_BRICK,

    TEXTURE_ID_COUNT,
} Texture_ID;

const char *get_texture_path(Texture_ID id);

#endif /* TEXTURES_H */
