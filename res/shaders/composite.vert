#version 430

out vec2 v_uv;

void main() {
    v_uv = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    v_uv.y = 1.0 - v_uv.y;

    gl_Position = vec4(v_uv * vec2(2, -2) + vec2(-1, 1), 0, 1);
}