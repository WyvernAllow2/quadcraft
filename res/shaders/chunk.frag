#version 430
layout(location = 0) out vec4 v_frag;

in vec3 v_normal;
in vec2 v_uv;
flat in int v_texture;

uniform sampler2DArray u_textures;

void main() {
    vec4 color = texture(u_textures, vec3(v_uv, v_texture));

    v_frag = vec4(color.rgb, 1.0);
}