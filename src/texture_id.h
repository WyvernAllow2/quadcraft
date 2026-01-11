#ifndef TEXTURE_ID
#define TEXTURE_ID

#define TEXTURE_SIZE 16

typedef enum Texture_ID {
    TEXTURE_ID_DIRT,

    TEXTURE_ID_COUNT,
} Texture_ID;

const char *get_texture_filename(Texture_ID id);

#endif /* TEXTURE_ID */
