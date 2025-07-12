#version 330
out vec4 v_frag;

in vec3 v_normal;
in vec2 v_uv;
in float v_layer;

uniform sampler2DArray u_texture_array;

void main() {
    v_frag = texture(u_texture_array, vec3(v_uv, v_layer));
}
