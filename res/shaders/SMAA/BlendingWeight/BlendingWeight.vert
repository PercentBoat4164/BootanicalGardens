#version 460

#include "../../BooLib.glsl"


layout (set=PER_PASS_SET, binding=0) uniform Resolution {
    vec4 resolution;
};

layout (constant_id = 1) const uint SMAA_MaxSearchSteps = 32;

layout (location = 0) out vec2 outTextureCoordinates;
layout (location = 1) out vec2 outPixelCoordinates;
layout (location = 2) out vec4 outOffset[3];

void main() {
    fullscreenTriangle(outTextureCoordinates);

    outPixelCoordinates = outTextureCoordinates * resolution.zw;

    // We will use these offsets for the searches later on (see @PSEUDO_GATHER4):
    outOffset[0] = fma(resolution.xyxy, vec4(-0.25,  -0.125,  1.25,  -0.125), outTextureCoordinates.xyxy);
    outOffset[1] = fma(resolution.xyxy, vec4(-0.125, -0.25,  -0.125,  1.25), outTextureCoordinates.xyxy);

    // And these for the searches, they indicate the ends of the loops:
    outOffset[2] = fma(resolution.xxyy, vec4(-2.0, 2.0, -2.0, 2.0) * SMAA_MaxSearchSteps, vec4(outOffset[0].xz, outOffset[1].yw));
}