#version 460

layout (location = 1) in vec2 inTextureCoordinates;

layout (location = 0) out vec4 color;

void main() {
    color.xy = inTextureCoordinates;
}