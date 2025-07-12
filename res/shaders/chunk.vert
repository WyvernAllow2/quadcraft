#version 330
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in float a_layer;

uniform mat4 u_view;
uniform mat4 u_proj;

out vec3 v_normal;
out vec2 v_uv;
out float v_layer;

vec2 compute_uv(vec3 position, vec3 normal) {
    return vec2(dot(normal.xzy, position.zxx), position.y + normal.y * position.z);
}

void main() {
    gl_Position = u_proj * u_view * vec4(a_position, 1.0);

    v_normal = a_normal;
    v_uv = compute_uv(a_position, a_normal);
    v_layer = a_layer;
}