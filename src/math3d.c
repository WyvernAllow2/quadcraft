#include "math3d.h"

#include <math.h>

float to_radians(float degrees) {
    return degrees * (PI / 180.0f);
}

float to_degrees(float radians) {
    return radians * (180.0f / PI);
}

float clamp(float value, float min, float max) {
    if (value < min) {
        return min;
    }
    if (value > max) {
        return max;
    }
    return value;
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
        return (Vec3){0};
    }

    return vec3_scale(v, 1.0f / len);
}

void mat4_identity(Mat4 *out) {
    *out = (Mat4){0};

    out->data[0] = 1.0f;
    out->data[5] = 1.0f;
    out->data[10] = 1.0f;
    out->data[15] = 1.0f;
}

void mat4_look_at(Mat4 *out, Vec3 eye, Vec3 center, Vec3 up) {
    Vec3 f = vec3_normalize(vec3_sub(center, eye));
    Vec3 s = vec3_normalize(vec3_cross(f, up));
    Vec3 u = vec3_cross(s, f);

    mat4_identity(out);

    out->data[0] = s.x;
    out->data[1] = u.x;
    out->data[2] = -f.x;
    out->data[4] = s.y;
    out->data[5] = u.y;
    out->data[6] = -f.y;
    out->data[8] = s.z;
    out->data[9] = u.z;
    out->data[10] = -f.z;

    out->data[12] = -vec3_dot(s, eye);
    out->data[13] = -vec3_dot(u, eye);
    out->data[14] = vec3_dot(f, eye);
}

void mat4_perspective(Mat4 *out, float fov, float aspect, float near, float far) {
    *out = (Mat4){0};

    float f = 1.0f / tanf(fov / 2.0f);
    float nf = 1.0f / (near - far);

    out->data[0] = f / aspect;
    out->data[5] = f;
    out->data[10] = (far + near) * nf;
    out->data[11] = -1.0f;
    out->data[14] = (2.0f * far * near) * nf;
    out->data[15] = 0.0f;
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

Vec3 vec3_from_ivec3(iVec3 v) {
    return (Vec3){
        .x = (float)v.x,
        .y = (float)v.y,
        .z = (float)v.z,
    };
}

iVec3 ivec3_from_vec3(Vec3 v) {
    return (iVec3){
        .x = (int)v.x,
        .y = (int)v.y,
        .z = (int)v.z,
    };
}
