#version 460

layout(push_constant) uniform MaterialID { uint materialID; };

void main() {
    gl_Position = vec4(vec2(gl_VertexIndex & 2, (gl_VertexIndex << 1) & 2) * vec2(2, -2) + vec2(-1, 1), uintBitsToFloat(materialID), 1);
}