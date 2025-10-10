#version 430
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in int a_texture;

uniform mat4 u_view;
uniform mat4 u_proj;

out vec3 v_normal;
out vec2 v_uv;

flat out int v_texture;

uniform ivec3 u_chunk_position;

out vec3 v_camera_position;
out vec3 v_position;

void main() {
    mat4 inv_view = inverse(u_view);
    v_camera_position = vec3(inv_view[3].xyz);

    v_position = a_position + u_chunk_position * 32.0;

    gl_Position = u_proj * u_view * vec4(v_position, 1.0);
    v_normal = a_normal;

    v_uv = vec2(dot(a_normal.xzy, a_position.zxx), a_position.y + a_normal.y * a_position.z);
    v_texture = a_texture;
}