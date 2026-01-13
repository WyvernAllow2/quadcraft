#version 430
layout(location = 0) out vec4 v_frag;

in vec3 v_position;
in vec3 v_normal;
in vec3 v_uv;
in vec3 v_color;

uniform sampler2DArray u_texture_array;
uniform vec3 u_camera_position;

uniform vec3 u_sun_direction;
uniform vec3 u_sun_color;

uniform float u_ambient_strength;
uniform vec3 u_ambient_color;

uniform float u_fog_end;
uniform float u_fog_density;

void main() {
    vec3 albedo = texture(u_texture_array, v_uv).rgb * v_color;

    vec3 ambient = u_ambient_color * u_ambient_strength;

    float diffuse_strength = max(dot(u_sun_direction, v_normal), 0.0);
    vec3 diffuse = diffuse_strength * u_sun_color;

    float dist = distance(v_position, u_camera_position);
    float dist_ratio = 4.0 * dist / u_fog_end;
    float fog_factor = exp(-dist_ratio * u_fog_density);

    vec3 color = albedo * (ambient + diffuse);
    vec3 final = mix(u_ambient_color, color, fog_factor);

    v_frag = vec4(final, 1.0);
}