#include "direction.h"

/* clang-format off */
static const Vec3 DIRECTION_TO_VEC3_TABLE[DIRECTION_COUNT] = {
    [DIR_POS_X] = { 1.0f,  0.0f,  0.0f},
    [DIR_POS_Y] = { 0.0f,  1.0f,  0.0f},
    [DIR_POS_Z] = { 0.0f,  0.0f,  1.0f},
    [DIR_NEG_X] = {-1.0f,  0.0f,  0.0f},
    [DIR_NEG_Y] = { 0.0f, -1.0f,  0.0f},
    [DIR_NEG_Z] = { 0.0f,  0.0f, -1.0f},
};

static const iVec3 DIRECTION_TO_IVEC3_TABLE[DIRECTION_COUNT] = {
    [DIR_POS_X] = { 1,  0,  0},
    [DIR_POS_Y] = { 0,  1,  0},
    [DIR_POS_Z] = { 0,  0,  1},
    [DIR_NEG_X] = {-1,  0,  0},
    [DIR_NEG_Y] = { 0, -1,  0},
    [DIR_NEG_Z] = { 0,  0, -1},
};

/* clang-format on */

Vec3 direction_to_vec3(Direction dir) {
    return DIRECTION_TO_VEC3_TABLE[dir];
}

iVec3 direction_to_ivec3(Direction dir) {
    return DIRECTION_TO_IVEC3_TABLE[dir];
}
