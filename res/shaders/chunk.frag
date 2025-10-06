#version 430
layout(location = 0) out vec4 v_frag;

in vec3 v_normal;

void main() {
    v_frag = vec4(v_normal * 0.5 + 0.5, 1.0);
}