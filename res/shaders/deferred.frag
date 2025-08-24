#version 460
#include "BooLib.glsl"

layout (location = 0) in vec2 screenSpaceCoordinates;

layout (set=PER_PASS_SET, binding=0) uniform sampler2D worldSpacePosition;  /**@todo: Convert me to an input attachment.*/

layout (set=PER_MATERIAL_SET, binding=0) uniform LightData {
    mat4 light_ViewProjectionMatrix;
} lightData;
layout (set=PER_MATERIAL_SET, binding=1) uniform sampler2D shadowMap;

layout (location = 0) out vec4 outColor;

void main() {
    // Transform the world space position of the fragment to the light's Clip Space.
    vec4 shadowMapPosition = lightData.light_ViewProjectionMatrix * texture(worldSpacePosition, screenSpaceCoordinates);
    // Transform the fragment's light Clip Space position into the light's NDC Space.
    shadowMapPosition /= shadowMapPosition.w;  // Perspective division (Not actually needed when using orthographic projection when rendering the shadow map because in that case w is 1.)
    // Transform the fragment's light NDC Space into the shadowMap's Image Space.
    float shadowDepth = texture(shadowMap, (shadowMapPosition.xy + 1.0) / 2.0).x;  // shadowDepth is in the light's NDC Space.
    // Both shadowMapPosition.z and shadowDepth are in the light view's NDC, so we can directly compare them.
    if (shadowMapPosition.z > shadowDepth) outColor = vec4(1.0, 1.0, 1.0, 1.0);
    else discard;  /**@todo: Avoid using `discard`. Use an input attachment to read the current GBufferAlbdeo instead.*/
}