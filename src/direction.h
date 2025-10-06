#ifndef DIRECTION_H
#define DIRECTION_H

#include "math3d.h"

typedef enum Direction {
    DIR_POSITIVE_X,
    DIR_POSITIVE_Y,
    DIR_POSITIVE_Z,
    DIR_NEGATIVE_X,
    DIR_NEGATIVE_Y,
    DIR_NEGATIVE_Z,

    DIRECTION_COUNT,
} Direction;

iVec3 direction_to_ivec3(Direction direction);

#endif /* DIRECTION_H */
