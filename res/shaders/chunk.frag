#version 430
layout(location = 0) out vec4 v_frag;

uniform sampler2DArray u_textures;
uniform vec3 u_camera_position;

vec3 sun_dir = vec3(0.5, 1.0, 0.5);
vec3 light_color = vec3(0.9, 0.8, 0.7);

in VS_OUT {
    vec3 position;
    vec3 normal;
    vec3 uv;
}
fs_in;

void main() {
    vec4 texel_color = texture(u_textures, fs_in.uv);

    float ambient_strength = 0.1;
    vec3 ambient = ambient_strength * vec3(0.7, 0.7, 0.9);

    float diffuse_strength = dot(sun_dir, fs_in.normal) * 0.5 + 0.5;
    vec3 diffuse = diffuse_strength * light_color;

    float fog_end = 200.0;
    float fog_density = 0.2;

    float dist = length(fs_in.position - u_camera_position);
    float dist_ratio = 4.0 * dist / fog_end;
    float fog_factor = exp(-dist_ratio * fog_density);

    vec3 final_color = texel_color.rgb * (ambient + diffuse);

    v_frag = vec4(mix(vec3(0.7, 0.7, 0.9), final_color, fog_factor), 1.0);
}