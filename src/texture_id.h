#ifndef TEXTURE_ID
#define TEXTURE_ID

#define TEXTURE_SIZE 16

typedef enum Texture_ID {
    TEXTURE_DIRT,
    TEXTURE_GRASS_SIDE,
    TEXTURE_GRASS,
    TEXTURE_STONE,
    TEXTURE_PLANK,
    TEXTURE_BRICK,
    TEXTURE_LOG_SIDE,
    TEXTURE_LOG_TOP,
    
    TEXTURE_ID_COUNT,
} Texture_ID;

const char *get_texture_filename(Texture_ID id);

#endif /* TEXTURE_ID */
