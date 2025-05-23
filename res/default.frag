#version 460
#include "BooLib.glsl"

layout (location = 0) in vec2 inTextureCoordinates0;

PER_MATERIAL_DATA_ALBEDO albedo;

layout (location = 0) out vec4 outColor;

void main() {
    outColor = texture(albedo, inTextureCoordinates0);
}