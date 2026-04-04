#version 460

#include "BooLib.glsl"

/** Per-vertex data **/
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec2 inTextureCoordinates;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inTangent;
/** Per-instance data **/
layout (location = 4) in mat4 inModelMatrix;  // Consumes locations 4, 5, 6, and 7
layout (location = 8) in float inMaterialID;

layout (set=PER_PASS_SET, binding=0) uniform PassData {
    mat4 viewProjectionMatrix;
} passData;

layout (location = 0) out vec2 outTextureCoordinates;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec3 outTangent;
layout (location = 3) flat out float outMaterialID;

void main() {
    outTextureCoordinates = inTextureCoordinates;
    outNormal = inNormal;
    outTangent = inTangent;
    outMaterialID = inMaterialID;
    gl_Position = passData.viewProjectionMatrix * inModelMatrix * vec4(inPosition, 1);
}