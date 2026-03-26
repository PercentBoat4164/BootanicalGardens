#version 460

#include "BooLib.glsl"

layout (location = 0) in vec2 inTextureCoordinates;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inTangent;
layout (location = 3) flat in float inMaterialID;

layout (location = 0) out vec2 gBufferTextureCoordinate;
layout (location = 1) out vec3 gBufferNormal;
layout (location = 2) out vec3 gBufferTangent;
layout (location = 3) out float gBufferMaterialID;

void main() {
    gBufferTextureCoordinate = inTextureCoordinates;
    gBufferNormal = inNormal;
    gBufferTangent = inTangent;
    gBufferMaterialID = inMaterialID;
}