#version 460

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 textureCoordinates;

layout (binding = 0) uniform PVMData {
    mat4 pvmMat;
};

layout (location = 0) out vec2 outUV;

void main() {
    outUV = textureCoordinates;
    gl_Position = pvmMat * vec4(position, 1);
}