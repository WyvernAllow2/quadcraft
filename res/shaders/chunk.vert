#version 430
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;

uniform mat4 u_view;
uniform mat4 u_proj;

out vec3 v_normal;

void main() {
    gl_Position = u_proj * u_view * vec4(a_position, 1.0);
    v_normal = a_normal;
}