#version 430
layout(location = 0) in uint a_vertex;

uniform mat4 u_view;
uniform mat4 u_proj;
uniform ivec3 u_chunk_position;

out VS_OUT {
    vec3 position;
    vec3 normal;
    vec3 uv;
}
vs_out;

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
    uint texture_id = a_vertex & 0x7FFu;
    uint direction = (a_vertex >> 11) & 0x7u;
    uint z = (a_vertex >> 14) & 0x3Fu;
    uint y = (a_vertex >> 20) & 0x3Fu;
    uint x = (a_vertex >> 26) & 0x3Fu;

    vec3 position = vec3(x, y, z) + u_chunk_position * 32;
    vec3 normal = NORMAL_TABLE[direction];
    vec2 uv = vec2(dot(normal.xzy, position.zxx), position.y + normal.y * position.z);

    vs_out.position = position;
    vs_out.normal = normal;
    vs_out.uv = vec3(uv, float(texture_id));

    gl_Position = u_proj * u_view * vec4(position, 1.0);
}