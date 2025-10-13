#ifndef MATH3D_H
#define MATH3D_H
#include <math.h>

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

typedef struct iVec3 {
    int x;
    int y;
    int z;
} iVec3;

typedef struct Mat4 {
    float data[4 * 4];
} Mat4;

float to_radians(float degrees);
float to_degrees(float radians);

float clamp(float value, float min, float max);
float sign(float x);
float lerp(float a, float b, float t);
float smooth_damp(float a, float b, float k, float dt);

int mod(int a, int b);

Vec3 vec3_add(Vec3 a, Vec3 b);
Vec3 vec3_sub(Vec3 a, Vec3 b);
Vec3 vec3_scale(Vec3 v, float s);
Vec3 vec3_cross(Vec3 a, Vec3 b);
float vec3_dot(Vec3 a, Vec3 b);
float vec3_len_squared(Vec3 v);
float vec3_len(Vec3 v);
Vec3 vec3_normalize(Vec3 v);
Vec3 vec3_sign(Vec3 v);
Vec3 vec3_abs(Vec3 v);

Vec4 vec4_add(Vec4 a, Vec4 b);
Vec4 vec4_sub(Vec4 a, Vec4 b);
Vec4 vec4_scale(Vec4 v, float s);
float vec4_dot(Vec4 a, Vec4 b);
float vec4_len_squared(Vec4 v);
float vec4_len(Vec4 v);
Vec4 vec4_normalize(Vec4 v);

void mat4_identity(Mat4 *out);
void mat4_look_at(Mat4 *out, Vec3 eye, Vec3 center, Vec3 up);
void mat4_perspective(Mat4 *out, float fov, float aspect, float znear, float zfar);
void mat4_mul(Mat4 *out, const Mat4 *a, const Mat4 *b);
void mat4_transpose(Mat4 *out, const Mat4 *m);
Vec4 mat4_column(const Mat4 *m, int column);

Vec3 ivec3_to_vec3(iVec3 v);
iVec3 vec3_to_ivec3(Vec3 v);

iVec3 ivec3_add(iVec3 a, iVec3 b);
iVec3 ivec3_sub(iVec3 a, iVec3 b);
iVec3 ivec3_mul(iVec3 v, int s);
iVec3 ivec3_div(iVec3 v, int s);
iVec3 ivec3_mod(iVec3 v, int x);
iVec3 ivec3_abs(iVec3 v);
float ivec3_len_squared(iVec3 v);
float ivec3_len(iVec3 v);

#endif /* MATH3D_H */
