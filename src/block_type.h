#ifndef BLOCK_TYPE_H
#define BLOCK_TYPE_H

#include <stdbool.h>

#include "textures.h"
#include "direction.h"

typedef enum Block_Type {
    BLOCK_AIR,
    BLOCK_DIRT,
    BLOCK_STONE,
    BLOCK_GRASS,
    BLOCK_LOG,
    BLOCK_PLANK,
    BLOCK_BRICK,

    BLOCK_TYPE_COUNT,
} Block_Type;

typedef struct Block_Properties {
    bool is_transparent;
    Texture_ID face_textures[DIRECTION_COUNT];
} Block_Properties;

const Block_Properties *get_block_properties(Block_Type type);

#endif /* BLOCK_TYPE_H */
