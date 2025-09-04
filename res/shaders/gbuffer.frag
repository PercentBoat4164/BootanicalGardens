#version 460
#include "BooLib.glsl"

layout (location = 0) in vec2 inTextureCoordinates;
layout (location = 1) in vec3 inWorldSpacePosition;

layout (set=PER_MATERIAL_SET, binding=0) uniform sampler2D albedo;

layout (location = 0) out vec4 gBufferAlbedo;
layout (location = 1) out vec4 gBufferPosition;

void main() {
    gBufferPosition = vec4(inWorldSpacePosition, 1.0);
    gBufferAlbedo = texture(albedo, inTextureCoordinates);
}