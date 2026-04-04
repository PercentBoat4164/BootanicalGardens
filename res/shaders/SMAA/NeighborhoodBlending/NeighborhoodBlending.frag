#version 460

#include "../../BooLib.glsl"

layout (location = 0) in vec2 inTextureCoordinates;
layout (location = 1) in vec4 inOffset;

layout (set=PER_PASS_SET, binding=0) uniform Resolution {
    vec4 resolution;
};

layout (set=PER_MATERIAL_SET, binding=0) uniform sampler2D SMAABlend;
layout (set=PER_MATERIAL_SET, binding=1) uniform sampler2D renderColor;

layout (location = 0) out vec4 SMAAResults;


/**
 * Conditional move:
 */
void cmov(bvec2 cond, inout vec2 variable, vec2 value) {
    if (cond.x) variable.x = value.x;
    if (cond.y) variable.y = value.y;
}

void cmov(bvec4 cond, inout vec4 variable, vec4 value) {
    cmov(cond.xy, variable.xy, value.xy);
    cmov(cond.zw, variable.zw, value.zw);
}

void main() {
    // Fetch the blending weights for current pixel:
    vec4 a;
    a.x = texture(SMAABlend, inOffset.xy).a; // Right
    a.y = texture(SMAABlend, inOffset.zw).g; // Top
    a.wz = texture(SMAABlend, inTextureCoordinates).xz; // Bottom / Left

    // Is there any blending weight with a value greater than 0.0?
    if (dot(a, vec4(1.0, 1.0, 1.0, 1.0)) == 0) {
        SMAAResults = textureLod(renderColor, inTextureCoordinates, 0);

        #if SMAA_REPROJECTION
//        vec2 velocity = SMAA_DECODE_VELOCITY(SMAASampleLevelZero(velocityTex, inTextureCoordinates));
//
//        // Pack velocity into the alpha channel:
//        SMAAResults.a = sqrt(5.0 * length(velocity));
        #endif
    } else {
        bool h = max(a.x, a.z) > max(a.y, a.w); // max(horizontal) > max(vertical)

        // Calculate the blending offsets:
        vec4 blendingOffset = vec4(0.0, a.y, 0.0, a.w);
        vec2 blendingWeight = a.yw;
        cmov(bvec4(h, h, h, h), blendingOffset, vec4(a.x, 0.0, a.z, 0.0));
        cmov(bvec2(h, h), blendingWeight, a.xz);
        blendingWeight /= dot(blendingWeight, vec2(1.0, 1.0));

        // Calculate the texture coordinates:
        vec4 blendingCoord = fma(blendingOffset, vec4(resolution.xy, -resolution.xy), inTextureCoordinates.xyxy);

        // We exploit bilinear filtering to mix current pixel with the chosen
        // neighbor:
        SMAAResults = blendingWeight.x * textureLod(renderColor, blendingCoord.xy, 0.0);
        SMAAResults += blendingWeight.y * textureLod(renderColor, blendingCoord.zw, 0.0);

        #if SMAA_REPROJECTION
//        // Antialias velocity for proper reprojection in a later stage:
//        vec2 velocity = blendingWeight.x * SMAA_DECODE_VELOCITY(SMAASampleLevelZero(velocityTex, blendingCoord.xy));
//        velocity += blendingWeight.y * SMAA_DECODE_VELOCITY(SMAASampleLevelZero(velocityTex, blendingCoord.zw));
//
//        // Pack velocity into the alpha channel:
//        SMAAResults.a = sqrt(5.0 * length(velocity));
        #endif
    }
}