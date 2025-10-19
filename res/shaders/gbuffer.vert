#version 460
#include "BooLib.glsl"

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec2 inTextureCoordinates;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inTangent;
layout (location = 4) in mat4 inModelMatrix;  // Consumes locations 4, 5, 6, and 7
layout (location = 8) in uint inMaterialID;

layout (set=PER_PASS_SET, binding=0) uniform PassData {
    mat4 viewProjectionMatrix;
} passData;

layout (location = 0) out vec3 outWorldSpacePosition;
layout (location = 1) out vec2 outTextureCoordinates;
layout (location = 2) out vec3 outNormal;
layout (location = 3) out vec3 outTangent;
layout (location = 4) flat out uint outMaterialID;

void main() {
    outWorldSpacePosition = (inModelMatrix * vec4(inPosition, 1)).xyz;
    outTextureCoordinates = inTextureCoordinates;
    outNormal = inNormal;
    outTangent = inTangent;
    outMaterialID = inMaterialID;
    gl_Position = passData.viewProjectionMatrix * vec4(outWorldSpacePosition, 1);
}