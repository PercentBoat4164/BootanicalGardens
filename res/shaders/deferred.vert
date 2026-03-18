#version 460

#include "BooLib.glsl"


layout(push_constant) uniform MaterialID { float materialID; };

void main() {
    fullscreenTriangle(materialID);
}