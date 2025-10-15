#version 460

void main() {
    gl_Position = vec4(vec2(gl_VertexIndex & 2, (gl_VertexIndex << 1) & 2) * vec2(2, -2) + vec2(-1, 1), 0, 1);
}