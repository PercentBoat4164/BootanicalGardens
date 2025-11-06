#version 460
#include "BooLib.glsl"

layout (set=PER_FRAME_SET, binding=0) uniform FrameData {
    uint frameNumber;
    float time;
} frameData;

layout (input_attachment_index=0, set=PER_PASS_SET, binding=0) uniform subpassInput gBufferAlbedo;
layout (input_attachment_index=1, set=PER_PASS_SET, binding=1) uniform subpassInput gBufferPosition;
layout (input_attachment_index=2, set=PER_PASS_SET, binding=2) uniform subpassInput gBufferNormal;

layout (set=PER_MATERIAL_SET, binding=0) uniform LightData {
    mat4 light_ViewProjectionMatrix;
    vec3 light_Position;
} lightData;
layout (set=PER_MATERIAL_SET, binding=1) uniform sampler2D shadowMap;

layout (location = 0) out vec4 renderColor;

const float maxBias = 1.0 / 1024;  // The shadowMap's resolution is 1024x1024, so 1.0 / 1024 = the width of one texel in the shadowMap's Image Space.
const vec3 lightColor = vec3(1);  // The color of the light.
const float ambientLight = 0.05;  // The base light factor to add unconditionally. This is not effected by lightColor, and *can* make the output of the shader go above 1.0.

void main() {
    renderColor = subpassLoad(gBufferAlbedo);
    vec4 position = vec4(subpassLoad(gBufferPosition).xyz, 1);
    vec3 normal = subpassLoad(gBufferNormal).xyz;

    /*************************************************************************
     * Extract the depth value for the corresponding pixel in the shadow map *
     *************************************************************************/
    vec4 shadowMapPosition = lightData.light_ViewProjectionMatrix * position;  // Transform the world space position of the fragment to the light's Clip Space.
    shadowMapPosition /= shadowMapPosition.w;  // Transform the fragment's light Clip Space position into the light's NDC Space using perspective division (Not actually needed when using orthographic projection when rendering the shadow map because in that case w is 1).
    float shadowDepth = texture(shadowMap, (shadowMapPosition.xy + 1.0) / 2.0).x;  // Transform the fragment's light NDC Space position into the shadowMap's Image Space. ShadowDepth is in the light's NDC Space.

    /***************************
     * Compute the shadow bias *
     ***************************/
    vec3 fragmentToLightVector = normalize(lightData.light_Position - position.xyz);  // Compute a vector from the current fragment to the light in World Space.
    float cosine_Normal_fragmentToLight = max(dot(normal, fragmentToLightVector), 0);  // Compute the cosine of the angle betweeen the fragment normal and the vector to the light.
    float bias = maxBias * cosine_Normal_fragmentToLight;  // Multiply by the maxBias get the shadow bias.
    shadowMapPosition.z -= bias;  // Apply the bias by subtracting. Subtracting brings the depth from the shadow map to be closer to zero, in other words, closer to the light source.

    /*****************************************
     * Compute the lighting on this fragment *
     *****************************************/
    vec3 light = vec3(0);
    if (shadowMapPosition.z < shadowDepth) {  // Both shadowMapPosition.z and shadowDepth are in the light view's NDC and the shadowMapPosition.z has already had the bias applied, so we can directly compare them.
        /*********************************************************
         * The fragment is in the light, so compute the lighting *
         *********************************************************/
        float lightIntensity = cosine_Normal_fragmentToLight;  // Decrease the light intensity with the angle at which it hits the surface.
        light = lightColor * lightIntensity;  // The light value is the light's color times the light's intensity.
    }
    renderColor *= vec4(light + ambientLight, 1);  // Add ambient light and multiply by albedo to compute final color.
}