#version 460
#include "BooLib.glsl"

layout (location = 0) in vec2 inTextureCoordinates0;

PER_FRAME_DATA frameData;
PER_MATERIAL_DATA_ALBEDO albedo;

layout (location = 0) out vec4 outColor;

vec4 RGB_HSV(vec4 rgb) {
    float K = 0;
    float temp;
    if (rgb.g < rgb.b) {
        temp = rgb.g;
        rgb.g = rgb.b;
        rgb.b = temp;
        K = -1;
    }
    if (rgb.r < rgb.g) {
        temp = rgb.r;
        rgb.r = rgb.g;
        rgb.g = temp;
        K = -1.f / 6.f - K;
    }
    float chroma = rgb.r - min(rgb.g, rgb.b);
    return vec4(abs(K + (rgb.g - rgb.b) / (6 * chroma + .00000000001)), chroma / (rgb.r + .00000000001), rgb.r, rgb.a);
}

vec4 HSV_RGB(vec4 hsv) {
    vec3 rgb = clamp(abs(mod(hsv.x * 6.0 + vec3(0.0, 4.0, 2.0), 6.0) - 3.0) - 1.0, 0.0, 1.0);
    rgb = rgb * rgb * (3.0 - 2.0 * rgb); // Cubic smoothing
    return vec4(hsv.z * mix(vec3(1.0), rgb, hsv.y), hsv.w);
}

void main() {
    outColor = texture(albedo, inTextureCoordinates0);
    outColor = RGB_HSV(outColor);
    outColor.x = mod(outColor.x + frameData.time, 1.0);
    outColor = HSV_RGB(normalize(outColor));
}