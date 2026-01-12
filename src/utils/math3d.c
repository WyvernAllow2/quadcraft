#include "math3d.h"

#include <assert.h>
#include <math.h>

float to_radians(float degrees) {
    return degrees * (PI / 180.0f);
}

float to_degrees(float radians) {
    return radians * (180.0f / PI);
}

float clamp(float value, float min, float max) {
    assert(min < max);

    if (value < min) {
        return min;
    }

    if (value > max) {
        return max;
    }

    return value;
}

int mod(int a, int b) {
    int r = a % b;
    return r < 0 ? r + b : r;
}

int floor_div(int a, int b) {
    int div = a / b;
    if ((a < 0) && (a % b != 0)) {
        div--;
    }
    return div;
}

Vec3 vec3_add(Vec3 a, Vec3 b) {
    return (Vec3){
        .x = a.x + b.x,
        .y = a.y + b.y,
        .z = a.z + b.z,
    };
}

Vec3 vec3_sub(Vec3 a, Vec3 b) {
    return (Vec3){
        .x = a.x - b.x,
        .y = a.y - b.y,
        .z = a.z - b.z,
    };
}

Vec3 vec3_scale(Vec3 v, float s) {
    return (Vec3){
        .x = v.x * s,
        .y = v.y * s,
        .z = v.z * s,
    };
}

Vec3 vec3_cross(Vec3 a, Vec3 b) {
    return (Vec3){
        .x = a.y * b.z - a.z * b.y,
        .y = a.z * b.x - a.x * b.z,
        .z = a.x * b.y - a.y * b.x,
    };
}

float vec3_dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

float vec3_len_squared(Vec3 v) {
    return vec3_dot(v, v);
}

float vec3_len(Vec3 v) {
    return sqrtf(vec3_len_squared(v));
}

Vec3 vec3_normalize(Vec3 v) {
    float len = vec3_len(v);
    if (len < 1e-7) {
        return (Vec3){0.0f, 0.0f, 0.0f};
    }
    return vec3_scale(v, 1.0f / len);
}

iVec3 ivec3_add(iVec3 a, iVec3 b) {
    return (iVec3){
        .x = a.x + b.x,
        .y = a.y + b.y,
        .z = a.z + b.z,
    };
}

iVec3 ivec3_sub(iVec3 a, iVec3 b) {
    return (iVec3){
        .x = a.x - b.x,
        .y = a.y - b.y,
        .z = a.z - b.z,
    };
}

iVec3 ivec3_scale(iVec3 v, int s) {
    return (iVec3){
        .x = v.x * s,
        .y = v.y * s,
        .z = v.z * s,
    };
}

iVec3 ivec3_div(iVec3 v, int s) {
    return (iVec3){
        .x = v.x / s,
        .y = v.y / s,
        .z = v.z / s,
    };
}

iVec3 ivec3_mod(iVec3 a, int b) {
    return (iVec3){
        .x = mod(a.x, b),
        .y = mod(a.y, b),
        .z = mod(a.z, b),
    };
}

iVec3 ivec3_floor_div(iVec3 v, int s) {
    return (iVec3){
        floor_div(v.x, s),
        floor_div(v.y, s),
        floor_div(v.z, s),
    };
}

void mat4_identity(Mat4 *out) {
    assert(out != NULL);
    /* clang-format off */
    *out = (Mat4){
        {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
        }
    };
    /* clang-format on */
}

void mat4_look_at(Mat4 *out, Vec3 eye, Vec3 center, Vec3 up) {
    assert(out != NULL);

    Vec3 f = vec3_normalize(vec3_sub(center, eye));
    Vec3 s = vec3_normalize(vec3_cross(f, up));
    Vec3 u = vec3_cross(s, f);
    Vec3 t = {
        .x = -vec3_dot(s, eye),
        .y = -vec3_dot(u, eye),
        .z = vec3_dot(f, eye),
    };

    /* clang-format off */
    *out = (Mat4){
        {
            s.x, u.x, -f.x, 0.0f,
            s.y, u.y, -f.y, 0.0f,
            s.z, u.z, -f.z, 0.0f,
            t.x, t.y,  t.z, 1.0f,
        }
    };
    /* clang-format on */
}

void mat4_mul(Mat4 *out, const Mat4 *a, const Mat4 *b) {
    assert(out != NULL);
    assert(a != NULL);
    assert(b != NULL);
    assert(out != a);
    assert(out != b);

    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k) {
                sum += a->data[k * 4 + row] * b->data[col * 4 + k];
            }
            out->data[col * 4 + row] = sum;
        }
    }
}

void mat4_perspective(Mat4 *out, float fovy, float aspect, float near, float far) {
    assert(out != NULL);
    assert(aspect != 0.0f);
    assert(far != near);

    float f = 1.0f / tanf(fovy * 0.5f);

    /* clang-format off */
    *out = (Mat4){
        {
            f / aspect, 0.0f, 0.0f, 0.0f,
            0.0f,      f,    0.0f, 0.0f,
            0.0f,      0.0f, (far + near) / (near - far), -1.0f,
            0.0f,      0.0f, (2.0f * far * near) / (near - far), 0.0f,
        }
    };
    /* clang-format on */
}
