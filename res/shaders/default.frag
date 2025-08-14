#version 460
#include "BooLib.glsl"

layout (location = 0) in vec2 inTextureCoordinates0;

layout (set=PER_MATERIAL_SET, binding=0) uniform sampler2D albedo;

layout (location = 0) out vec4 outColor;

void main() {
    outColor = texture(albedo, inTextureCoordinates0);
}