#version 430
layout(location = 0) out vec4 v_frag;

in vec3 v_normal;
in vec2 v_uv;
flat in int v_texture;

uniform sampler2DArray u_textures;

vec3 sun_dir = vec3(0.5, 1.0, 0.5);
vec3 light_color = vec3(0.9, 0.8, 0.7);

void main() {
    vec4 color = texture(u_textures, vec3(v_uv, v_texture));

    float ambient_strength = 0.1;
    vec3 ambient = ambient_strength * vec3(0.7, 0.7, 0.9);

    float diffuse_strength = dot(sun_dir, v_normal) * 0.5 + 0.5;
    vec3 diffuse = diffuse_strength * light_color;

    v_frag = vec4(color.rgb * (ambient + diffuse), 1.0);
}