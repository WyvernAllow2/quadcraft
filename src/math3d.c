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
    assert(min <= max);

    if (value < min) {
        return min;
    }
    if (value > max) {
        return max;
    }
    return value;
}

float sign(float x) {
    return (x > 0) - (x < 0);
}

float lerp(float a, float b, float t) {
    return (1 - t) * a + t * b;
}

float smooth_damp(float a, float b, float k, float dt) {
    return lerp(a, b, 1.0f - powf(k, dt));
}

int mod(int a, int b) {
    int r = a % b;
    return r < 0 ? r + b : r;
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

Vec3 vec3_sign(Vec3 v) {
    return (Vec3){
        .x = sign(v.x),
        .y = sign(v.y),
        .z = sign(v.z),
    };
}

Vec3 vec3_abs(Vec3 v) {
    return (Vec3){
        .x = fabsf(v.x),
        .y = fabsf(v.y),
        .z = fabsf(v.z),
    };
}

Vec4 vec4_add(Vec4 a, Vec4 b) {
    return (Vec4){
        .x = a.x + b.x,
        .y = a.y + b.y,
        .z = a.z + b.z,
        .w = a.w + b.w,
    };
}

Vec4 vec4_sub(Vec4 a, Vec4 b) {
    return (Vec4){
        .x = a.x - b.x,
        .y = a.y - b.y,
        .z = a.z - b.z,
        .w = a.w - b.w,
    };
}

Vec4 vec4_scale(Vec4 v, float s) {
    return (Vec4){
        .x = v.x * s,
        .y = v.y * s,
        .z = v.z * s,
        .w = v.w * s,
    };
}

float vec4_dot(Vec4 a, Vec4 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

float vec4_len_squared(Vec4 v) {
    return vec4_dot(v, v);
}

float vec4_len(Vec4 v) {
    return sqrtf(vec4_len_squared(v));
}

Vec4 vec4_normalize(Vec4 v) {
    float len = vec4_len(v);
    if (len < 1e-7) {
        return (Vec4){0.0f, 0.0f, 0.0f, 0.0f};
    }
    return vec4_scale(v, 1.0f / len);
}

void mat4_identity(Mat4 *out) {
    assert(out);
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
    assert(out);

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

void mat4_perspective(Mat4 *out, float fov, float aspect, float znear, float zfar) {
    assert(out);

    float zf = zfar;
    float zn = znear;
    float f = 1.0f / tanf(fov / 2.0f);
    float nf = 1.0f / (znear - zfar);
    float a = aspect;

    /* clang-format off */
    *out = (Mat4){
        {
            f / a,  0.0f,  0.0f,                   0.0f,
            0.0f,   f,     0.0f,                   0.0f,
            0.0f,   0.0f,  (zf + zn) * nf,        -1.0f,
            0.0f,   0.0f,  (2.0f * zf * zn) * nf,  0.0f
        }
    };
    /* clang-format on */
}

void mat4_mul(Mat4 *out, const Mat4 *a, const Mat4 *b) {
    assert(out);
    assert(a);
    assert(b);
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

void mat4_transpose(Mat4 *out, const Mat4 *m) {
    assert(out);
    assert(m);
    assert(out != m);

    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            out->data[col * 4 + row] = m->data[row * 4 + col];
        }
    }
}

Vec4 mat4_column(const Mat4 *m, int column) {
    assert(m);
    assert(column >= 0 && column < 4);

    return (Vec4){
        m->data[column * 4 + 0],
        m->data[column * 4 + 1],
        m->data[column * 4 + 2],
        m->data[column * 4 + 3],
    };
}

Vec3 ivec3_to_vec3(iVec3 v) {
    return (Vec3){
        .x = (float)v.x,
        .y = (float)v.y,
        .z = (float)v.z,
    };
}

iVec3 vec3_to_ivec3(Vec3 v) {
    return (iVec3){
        .x = (int)v.x,
        .y = (int)v.y,
        .z = (int)v.z,
    };
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

iVec3 ivec3_mul(iVec3 v, int s) {
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

iVec3 ivec3_mod(iVec3 v, int x) {
    return (iVec3){
        .x = mod(v.x, x),
        .y = mod(v.y, x),
        .z = mod(v.z, x),
    };
}

iVec3 ivec3_abs(iVec3 v) {
    return (iVec3){
        .x = abs(v.x),
        .y = abs(v.y),
        .z = abs(v.z),
    };
}

float ivec3_len_squared(iVec3 v) {
    return vec3_len_squared(ivec3_to_vec3(v));
}

float ivec3_len(iVec3 v) {
    return sqrtf(ivec3_len_squared(v));
}
