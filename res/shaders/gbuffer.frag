#version 460
#include "BooLib.glsl"

layout (location = 0) in vec3 inWorldSpacePosition;
layout (location = 1) in vec2 inTextureCoordinates;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inTangent;
layout (location = 4) flat in float inMaterialID;

layout (set=PER_MATERIAL_SET, binding=0) uniform sampler2D albedo;
layout (set=PER_MATERIAL_SET, binding=1) uniform sampler2D normal;

layout (location = 0) out vec4 gBufferAlbedo;
layout (location = 1) out vec3 gBufferPosition;
layout (location = 2) out vec3 gBufferNormal;
layout (location = 3) out float gBufferMaterialID;

void main() {
    gBufferAlbedo = texture(albedo, inTextureCoordinates);
    gBufferPosition = inWorldSpacePosition;
    vec3 N = normalize(inNormal);
    vec3 T = normalize(inTangent);
    vec3 B = cross(N, T);
    mat3 TBN = mat3(T, B, N);
    gBufferNormal = TBN * normalize(texture(normal, inTextureCoordinates).xyz);
    gBufferMaterialID = inMaterialID;
}