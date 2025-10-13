#include "aabb.h"

#include <assert.h>

bool aabb_vs_frustum(const AABB *aabb, const Frustum *frustum) {
    assert(aabb);
    assert(frustum);

    Vec3 min = aabb->position;
    Vec3 max = vec3_add(aabb->position, aabb->size);

    for (size_t i = 0; i < 6; ++i) {
        Vec4 plane = frustum->planes[i];

        if ((vec4_dot(plane, (Vec4){min.x, min.y, min.z, 1.0f}) < 0.0) &&
            (vec4_dot(plane, (Vec4){max.x, min.y, min.z, 1.0f}) < 0.0) &&
            (vec4_dot(plane, (Vec4){min.x, max.y, min.z, 1.0f}) < 0.0) &&
            (vec4_dot(plane, (Vec4){max.x, max.y, min.z, 1.0f}) < 0.0) &&
            (vec4_dot(plane, (Vec4){min.x, min.y, max.z, 1.0f}) < 0.0) &&
            (vec4_dot(plane, (Vec4){max.x, min.y, max.z, 1.0f}) < 0.0) &&
            (vec4_dot(plane, (Vec4){min.x, max.y, max.z, 1.0f}) < 0.0) &&
            (vec4_dot(plane, (Vec4){max.x, max.y, max.z, 1.0f}) < 0.0)) {
            return false;
        }
    }

    return true;
}

Vec3 aabb_min(const AABB *aabb) {
    assert(aabb);
    return aabb->position;
}

Vec3 aabb_max(const AABB *aabb) {
    assert(aabb);
    return vec3_add(aabb->position, aabb->size);
}

bool aabb_vs_aabb(const AABB *a, const AABB *b) {
    assert(a);
    assert(b);

    Vec3 a_min = aabb_min(a);
    Vec3 a_max = aabb_max(a);
    Vec3 b_min = aabb_min(b);
    Vec3 b_max = aabb_max(b);

    if (a_max.x < b_min.x || a_min.x > b_max.x) {
        return false;
    }

    if (a_max.y < b_min.y || a_min.y > b_max.y) {
        return false;
    }

    if (a_max.z < b_min.z || a_min.z > b_max.z) {
        return false;
    }

    return true;
}
