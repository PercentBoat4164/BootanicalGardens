#version 460
#include "BooLib.glsl"

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec2 inTextureCoordinates;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inTangent;

/**@todo: Remove this without causing validation errors.*/
layout (set=PER_FRAME_SET, binding=0) uniform FrameData {
    uint frameNumber;
    float time;
} frameData;

layout (set=PER_PASS_SET, binding=0) uniform PassData {
    mat4 viewProjectionMatrix;
} passData;

layout (location = 0) out vec3 outWorldSpacePosition;
layout (location = 1) out vec2 outTextureCoordinates;
layout (location = 2) out vec3 outNormal;
layout (location = 3) out vec3 outTangent;

void main() {
    outWorldSpacePosition = inPosition;
    outTextureCoordinates = inTextureCoordinates;
    outNormal = inNormal;
    outTangent = inTangent;
    gl_Position = passData.viewProjectionMatrix * vec4(outWorldSpacePosition, 1);
}