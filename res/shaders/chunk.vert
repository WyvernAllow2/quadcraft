#version 430
layout(location = 0) in uint a_vertex;

uniform mat4 u_view_proj;

out vec3 v_normal;
out vec3 v_uv;
out vec3 v_color;

/* clang-format off */
const vec3 NORMAL_TABLE[6] = vec3[](
    vec3( 1,  0,  0),    /* DIR_POSITIVE_X */
    vec3( 0,  1,  0),    /* DIR_POSITIVE_Y */
    vec3( 0,  0,  1),    /* DIR_POSITIVE_Z */
    vec3(-1,  0,  0),    /* DIR_NEGATIVE_X */
    vec3( 0, -1,  0),    /* DIR_NEGATIVE_Y */
    vec3( 0,  0, -1)     /* DIR_NEGATIVE_Z */
);
/* clang-format on */

void main() {
    uint x = (a_vertex >> 26) & 0x3Fu;
    uint y = (a_vertex >> 20) & 0x3Fu;
    uint z = (a_vertex >> 14) & 0x3Fu;
    uint direction = (a_vertex >> 11) & 0x7u;
    uint ao = (a_vertex >> 9) & 0x3u;
    uint texture_id = a_vertex & 0x1FFu;

    vec3 position = vec3(x, y, z);
    v_normal = NORMAL_TABLE[direction];
    v_uv = vec3(dot(v_normal.xzy, position.zxx), position.y + v_normal.y * position.z, texture_id);
    v_color = clamp(0.2 + vec3(ao / 4.0), 0.0, 1.0);

    gl_Position = u_view_proj * vec4(position, 1.0);
}