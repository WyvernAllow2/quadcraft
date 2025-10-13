#ifndef CAMERA_H
#define CAMERA_H

#include "math3d.h"

typedef struct Camera {
    Vec3 position;
    float pitch;
    float yaw;
    float roll;

    float fov;
    float aspect;
    float znear;
    float zfar;

    Vec3 forward;
    Vec3 right;
    Vec3 up;

    Mat4 view;
    Mat4 proj;
    Mat4 view_proj;
} Camera;

typedef struct Frustum {
    Vec4 planes[6];
} Frustum;

void camera_update(Camera *camera);
void camera_get_frustum(const Camera *camera, Frustum *out_frustum);

#endif /* CAMERA_H */
