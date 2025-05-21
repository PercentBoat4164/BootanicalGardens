#version 460

layout (location = 0) in vec2 inTextureCoordinates0;

layout (set = 1, binding = 0) uniform PVMData {
    mat4 viewProjectionMatrix;
};

layout (location = 0) out vec4 outColor;

void main() {
    outColor = vec4(inTextureCoordinates0, 0, 1);
}