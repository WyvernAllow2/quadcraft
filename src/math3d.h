#ifndef MATH3D_H
#define MATH3D_H

#include <math.h>
#include <stdbool.h>

#define PI (3.14159265358979323846)
#define HALF_PI (0.5 * PI)
#define TAU (2.0 * PI)

typedef struct Vec3 {
    float x;
    float y;
    float z;
} Vec3;

typedef struct Vec4 {
    float x;
    float y;
    float z;
    float w;
} Vec4;

typedef struct Mat4 {
    float data[4 * 4];
} Mat4;

float to_radians(float degrees);
float to_degrees(float radians);

Vec3 vec3_add(Vec3 a, Vec3 b);
Vec3 vec3_sub(Vec3 a, Vec3 b);
Vec3 vec3_scale(Vec3 v, float s);
Vec3 vec3_cross(Vec3 a, Vec3 b);
float vec3_dot(Vec3 a, Vec3 b);
float vec3_len_squared(Vec3 v);
float vec3_len(Vec3 v);
Vec3 vec3_normalize(Vec3 v);

void mat4_identity(Mat4 *out);
void mat4_look_at(Mat4 *out, Vec3 eye, Vec3 center, Vec3 up);
void mat4_perspective(Mat4 *out, float fov, float aspect, float znear, float zfar);
void mat4_mul(Mat4 *out, const Mat4 *a, const Mat4 *b);

#endif /* MATH3D_H */
