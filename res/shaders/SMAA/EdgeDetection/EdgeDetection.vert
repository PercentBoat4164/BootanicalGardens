#version 460

#include "../../BooLib.glsl"


layout (set=PER_PASS_SET, binding=0) uniform Resolution {
    vec4 resolution;
};

layout (location = 0) out vec2 outTextureCoordinates;
layout (location = 1) out vec4 outOffset[3];

void main() {
    fullscreenTriangle(outTextureCoordinates);

    outOffset[0] = fma(resolution.xyxy, vec4(-1.0, 0.0, 0.0, -1.0), outTextureCoordinates.xyxy);
    outOffset[1] = fma(resolution.xyxy, vec4( 1.0, 0.0, 0.0,  1.0), outTextureCoordinates.xyxy);
    outOffset[2] = fma(resolution.xyxy, vec4(-2.0, 0.0, 0.0, -2.0), outTextureCoordinates.xyxy);
}