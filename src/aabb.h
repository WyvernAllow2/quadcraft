#ifndef AABB_H
#define AABB_H

#include <stdbool.h>

#include "camera.h"
#include "math3d.h"

typedef struct AABB {
    Vec3 position;
    Vec3 size;
} AABB;

bool aabb_vs_frustum(const AABB *aabb, const Frustum *frustum);
Vec3 aabb_min(const AABB *aabb);
Vec3 aabb_max(const AABB *aabb);
bool aabb_vs_aabb(const AABB *a, const AABB *b);

#endif /* AABB_H */
