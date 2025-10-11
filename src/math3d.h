#ifndef MATH3D_H
#define MATH3D_H
#include <math.h>

#define PI (3.14159265358979323846)
#define HALF_PI (0.5 * PI)
#define TAU (2.0 * PI)

float to_radians(float degrees);
float to_degrees(float radians);

float clamp(float value, float min, float max);
int mod(int a, int b);
float signf(float x);
int signi(int x);

typedef struct Vec3 {
    float x;
    float y;
    float z;
} Vec3;

Vec3 vec3_add(Vec3 a, Vec3 b);
Vec3 vec3_sub(Vec3 a, Vec3 b);
Vec3 vec3_scale(Vec3 v, float s);
Vec3 vec3_cross(Vec3 a, Vec3 b);

float vec3_dot(Vec3 a, Vec3 b);
float vec3_len_squared(Vec3 v);
float vec3_len(Vec3 v);
Vec3 vec3_normalize(Vec3 v);

typedef struct Mat4 {
    float data[4 * 4];
} Mat4;

void mat4_identity(Mat4 *out);
void mat4_look_at(Mat4 *out, Vec3 eye, Vec3 center, Vec3 up);
void mat4_perspective(Mat4 *out, float fov, float aspect, float near, float far);
void mat4_mul(Mat4 *out, const Mat4 *a, const Mat4 *b);

typedef struct iVec3 {
    int x;
    int y;
    int z;
} iVec3;

iVec3 ivec3_add(iVec3 a, iVec3 b);
iVec3 ivec3_sub(iVec3 a, iVec3 b);
iVec3 ivec3_mod(iVec3 v, int x);
iVec3 ivec3_mul(iVec3 v, int s);
iVec3 ivec3_div(iVec3 v, int s);
iVec3 ivec3_sign(iVec3 v);

Vec3 vec3_from_ivec3(iVec3 v);
iVec3 ivec3_from_vec3(Vec3 v);

#endif /* MATH3D_H */
