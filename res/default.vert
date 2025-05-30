#version 460
#include "BooLib.glsl"

layout (location = 0) in vec3 inPosition;
layout (location = 3) in vec2 inTextureCoordinates0;

PER_RENDERPASS_DATA passData;
PER_MESH_DATA meshData;

layout (location = 0) out vec2 outTextureCoordinates;

void main() {
    gl_Position = passData.viewProjectionMatrix * vec4(inPosition, 1);
    outTextureCoordinates = inTextureCoordinates0;
}