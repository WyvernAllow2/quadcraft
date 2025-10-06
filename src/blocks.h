#ifndef BLOCKS_H
#define BLOCKS_H

#include <stdbool.h>
#include <stdint.h>

typedef enum Block_Type {
    BLOCK_AIR,
    BLOCK_DIRT,
    BLOCK_STONE,
    BLOCK_GRASS,

    BLOCK_TYPE_COUNT,
} Block_Type;

typedef struct Block_Properties {
    bool is_transparent;
} Block_Properties;

const Block_Properties *get_block_properties(Block_Type type);

#endif /* BLOCKS_H */
