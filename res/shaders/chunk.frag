#version 430
layout(location = 0) out vec4 v_frag;

in vec3 v_normal;
in vec3 v_uv;
in vec3 v_color;

uniform sampler2DArray u_texture_array;

void main() {
    vec3 color = texture(u_texture_array, v_uv).rgb * v_color;
    v_frag = vec4(color, 1.0);
}