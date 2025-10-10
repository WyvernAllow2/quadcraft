#version 430
layout(location = 0) out vec4 v_frag;

in vec3 v_color;

void main() {
    v_frag = vec4(v_color, 1.0);
}