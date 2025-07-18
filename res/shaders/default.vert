#version 460
#include "BooLib.glsl"

layout (location = 0) in vec3 inPosition;
layout (location = 3) in vec2 inTextureCoordinates0;

layout (set=PER_FRAME_SET, binding=0) uniform FrameData {
    uint frameNumber;
    float time;
} frameData;

layout (set=PER_PASS_SET, binding=0) uniform PassData {
    mat4 viewProjectionMatrix;
} passData;

layout (location = 0) out vec2 outTextureCoordinates;

void main() {
    gl_Position = passData.viewProjectionMatrix * vec4(inPosition, 1);
    outTextureCoordinates = inTextureCoordinates0;
}