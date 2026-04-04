#version 460

#include "../../BooLib.glsl"

layout (set=PER_PASS_SET, binding=0) uniform Resolution {
    vec4 resolution;
};

layout (location = 0) out vec2 outTextureCoordinates;
layout (location = 1) out vec4 outOffset;

void main() {
    fullscreenTriangle(outTextureCoordinates);

    outOffset = fma(resolution.xyxy, vec4(1.0, 0.0, 0.0, 1.0), outTextureCoordinates.xyxy);
}