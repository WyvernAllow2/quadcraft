#ifndef BLOCK_TYPE_H
#define BLOCK_TYPE_H

#include <stdbool.h>

typedef enum Block_Type {
    BLOCK_AIR,
    BLOCK_DIRT,

    BLOCK_TYPE_COUNT,
} Block_Type;

typedef struct Block_Properties {
    bool is_transparent;
} Block_Properties;

const Block_Properties *get_block_properties(Block_Type type);

#endif /* BLOCK_TYPE_H */
