#version 430
out vec4 v_frag;
in vec2 v_uv;

uniform sampler2D u_texture;

void main() {
    v_frag = texture(u_texture, v_uv);
}