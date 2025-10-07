#ifndef TEXTURE_ID
#define TEXTURE_ID

#define TEXTURE_SIZE 16

typedef enum Texture_ID {
    TEXTURE_DIRT,
    TEXTURE_GRASS_SIDE,
    TEXTURE_GRASS,
    TEXTURE_STONE,

    TEXTURE_POSITIVE_X,
    TEXTURE_POSITIVE_Y,
    TEXTURE_POSITIVE_Z,
    TEXTURE_NEGATIVE_X,
    TEXTURE_NEGATIVE_Y,
    TEXTURE_NEGATIVE_Z,
    
    TEXTURE_ID_COUNT,
} Texture_ID;

const char *get_texture_filename(Texture_ID id);

#endif /* TEXTURE_ID */
