#version 460

layout (location = 0) in vec3 inPosition;
layout (location = 3) in vec2 inTextureCoordinates0;

layout (set = 1, binding = 0) uniform PVMData {
    mat4 viewProjectionMatrix;
};

layout (location = 0) out vec2 outTextureCoordinates;

void main() {
    gl_Position = viewProjectionMatrix * vec4(inPosition, 1);
    outTextureCoordinates = inTextureCoordinates0;
}