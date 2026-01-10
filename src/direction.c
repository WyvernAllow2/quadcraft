#include "direction.h"

#include <assert.h>

/* clang-format off */
static const iVec3 DIRECTION_TABLE[DIRECTION_COUNT] = {
    [DIR_POSITIVE_X] = { 1,  0,  0},
    [DIR_POSITIVE_Y] = { 0,  1,  0},
    [DIR_POSITIVE_Z] = { 0,  0,  1},
    [DIR_NEGATIVE_X] = {-1,  0,  0},
    [DIR_NEGATIVE_Y] = { 0, -1,  0},
    [DIR_NEGATIVE_Z] = { 0,  0, -1},
};
/* clang-format on */

iVec3 direction_to_ivec3(Direction direction) {
    assert(direction >= 0 && direction < DIRECTION_COUNT);
    return DIRECTION_TABLE[direction];
}
