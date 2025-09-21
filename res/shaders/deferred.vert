#version 460
#include "BooLib.glsl"

layout (location = 0) out vec2 screenSpaceCoordinates;

void main() {
    screenSpaceCoordinates = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(screenSpaceCoordinates * vec2(2, -2) + vec2(-1, 1), 0, 1);
    screenSpaceCoordinates.y = 1 - screenSpaceCoordinates.y;
}