#version 460
#include "BooLib.glsl"

layout (location = 0) in vec3 inPosition;

layout (set=PER_PASS_SET, binding=0) uniform PassData {
    mat4 view_ViewProjectionMatrix;
} passData;

void main() {
    gl_Position = passData.view_ViewProjectionMatrix * vec4(inPosition, 1);
}