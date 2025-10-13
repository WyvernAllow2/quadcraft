#include "camera.h"

#include <assert.h>

void camera_update(Camera *camera) {
    assert(camera);

    const Vec3 WORLD_UP = {0.0f, 1.0f, 0.0f};

    camera->pitch = clamp(camera->pitch, -HALF_PI + 1e-6f, HALF_PI - 1e-6f);
    camera->yaw = fmodf(camera->yaw + TAU, TAU);

    camera->forward = vec3_normalize((Vec3){
        .x = cosf(camera->yaw) * cosf(camera->pitch),
        .y = sinf(camera->pitch),
        .z = sinf(camera->yaw) * cosf(camera->pitch),
    });

    camera->right = vec3_normalize(vec3_cross(camera->forward, WORLD_UP));
    camera->up = vec3_normalize(vec3_cross(camera->right, camera->forward));

    float cos_r = cosf(camera->roll);
    float sin_r = sinf(camera->roll);

    Vec3 right = camera->right;
    Vec3 up = camera->up;

    camera->right = vec3_add(vec3_scale(right, cos_r), vec3_scale(up, sin_r));
    camera->up = vec3_add(vec3_scale(up, cos_r), vec3_scale(right, -sin_r));

    Vec3 center = vec3_add(camera->position, camera->forward);

    mat4_look_at(&camera->view, camera->position, center, camera->up);
    mat4_perspective(&camera->proj, camera->fov, camera->aspect, camera->znear, camera->zfar);
    mat4_mul(&camera->view_proj, &camera->proj, &camera->view);
}

void camera_get_frustum(const Camera *camera, Frustum *out_frustum) {
    assert(camera);
    assert(out_frustum);

    Mat4 vpt;
    mat4_transpose(&vpt, &camera->view_proj);

    out_frustum->planes[0] = vec4_add(mat4_column(&vpt, 3), mat4_column(&vpt, 0));
    out_frustum->planes[1] = vec4_sub(mat4_column(&vpt, 3), mat4_column(&vpt, 0));
    out_frustum->planes[2] = vec4_add(mat4_column(&vpt, 3), mat4_column(&vpt, 1));
    out_frustum->planes[3] = vec4_sub(mat4_column(&vpt, 3), mat4_column(&vpt, 1));
    out_frustum->planes[4] = vec4_add(mat4_column(&vpt, 3), mat4_column(&vpt, 2));
    out_frustum->planes[5] = vec4_sub(mat4_column(&vpt, 3), mat4_column(&vpt, 2));
}
