#ifndef DIRECTION_H
#define DIRECTION_H

#include "math3d.h"

typedef enum Direction {
    DIR_POS_X,
    DIR_POS_Y,
    DIR_POS_Z,
    DIR_NEG_X,
    DIR_NEG_Y,
    DIR_NEG_Z,
    
    DIRECTION_COUNT,
} Direction;

Vec3 direction_to_vec3(Direction dir);
iVec3 direction_to_ivec3(Direction dir);

#endif /* DIRECTION_H */
