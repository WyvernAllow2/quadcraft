#include "camera.h"

void camera_update(Camera *camera) {
    const Vec3 WORLD_UP = {0.0f, 1.0f, 0.0f};

    camera->pitch = clamp(camera->pitch, -HALF_PI + 0.01f, HALF_PI - 0.01f);

    camera->forward = vec3_normalize((Vec3){
        .x = cosf(camera->yaw) * cosf(camera->pitch),
        .y = sinf(camera->pitch),
        .z = sinf(camera->yaw) * cosf(camera->pitch),
    });

    camera->right = vec3_normalize(vec3_cross(camera->forward, WORLD_UP));
    camera->up = vec3_normalize(vec3_cross(camera->right, camera->forward));

    mat4_look_at(&camera->view, camera->position, vec3_add(camera->position, camera->forward),
                 camera->up);
    mat4_perspective(&camera->proj, camera->fov, camera->aspect, camera->near, camera->far);
}
