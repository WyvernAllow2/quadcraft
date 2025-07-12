#version 330
out vec4 v_frag;

in vec3 v_normal;
in vec2 v_uv;

void main() {
    v_frag = vec4(fract(v_uv), 0, 1);
}
