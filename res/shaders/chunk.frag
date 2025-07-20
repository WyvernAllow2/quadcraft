#version 430
layout(location = 0) out vec3 g_diffuse;
layout(location = 1) out vec3 g_normal;

in vec3 v_normal;
in vec2 v_uv;
in float v_layer;

uniform sampler2DArray u_texture_array;

void main() {
    g_diffuse = texture(u_texture_array, vec3(v_uv, v_layer)).rgb;
    g_normal = v_normal * 0.5 + 0.5;
}
