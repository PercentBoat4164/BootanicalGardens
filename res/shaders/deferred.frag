#version 460

#include "BooLib.glsl"
#ifdef PARALLAX_MAPPING
#include "ParallaxLib.glsl"
#endif

layout (set=PER_FRAME_SET, binding=0) uniform FrameData {
    uint frameNumber;
    float time;
} frameData;

layout (input_attachment_index=0, set=PER_PASS_SET, binding=0) uniform subpassInput gBufferTextureCoordinate;
layout (input_attachment_index=2, set=PER_PASS_SET, binding=1) uniform subpassInput gBufferNormal;
layout (input_attachment_index=1, set=PER_PASS_SET, binding=2) uniform subpassInput gBufferTangent;
layout (input_attachment_index=3, set=PER_PASS_SET, binding=3) uniform subpassInput gBufferDepth;

layout (set=PER_PASS_SET, binding=4) uniform LightData {
    mat4 viewProjectionMatrix;
    vec3 position;
} lightData;
layout (set=PER_PASS_SET, binding=5) uniform ViewData {
    mat4 inverseViewProjectionMatrix;
    vec3 position;
    float _padding;
    vec2 resolution;
} viewData;

layout (set=PER_PASS_SET, binding=6) uniform sampler2D shadowMap;

layout (set=PER_MATERIAL_SET, binding=0) uniform sampler2D albedo;
layout (set=PER_MATERIAL_SET, binding=1) uniform sampler2D normal;
#ifdef PARALLAX_MAPPING
layout (set=PER_MATERIAL_SET, binding=2) uniform sampler2D displacement;
#endif

layout (location = 0) out vec4 renderColor;

const float maxBias = 1.0 / 1024;  // The shadowMap's resolution is 1024x1024, so 1.0 / 1024 = the width of one texel in the shadowMap's Image Space.
const vec3 lightColor = vec3(1);  // The color of the light.
const float ambientLight = 0.05;  // The base light factor to add unconditionally. This is not effected by lightColor, and *can* make the output of the shader go above 1.0.

void main() {
    /**************************************************
     * Compute the world space position of the sample *
     **************************************************/
    float reverseDepth = subpassLoad(gBufferDepth).x;  // Fetch the depth value from the reversed Z-buffer.
    const vec3 screenSpacePosition = vec3(gl_FragCoord.xy / viewData.resolution, reverseDepth);  // Builds the position of this sample in screen space. ([0, 1], [0, 1], [0, 1])
    const vec3 clipSpacePosition = vec3(screenSpacePosition.xy * 2.0 - 1.0, screenSpacePosition.z);  // Transforms from screen space into clip space. ([-1, 1], [-1, 1], [0, 1])
    const vec4 worldSpacePosition = viewData.inverseViewProjectionMatrix * vec4(clipSpacePosition, 1.0);  // Transform from clip space into world space.

    /**************************
     * Compute the TBN matrix *
     **************************/
    const vec3 N = normalize(subpassLoad(gBufferNormal).xyz);
    const vec3 T = normalize(subpassLoad(gBufferTangent).xyz);
    const mat3 TBN = mat3(T, cross(N, T), N);

    /************************************************
     * Compute the UVs of this point on the surface *
     ************************************************/
    vec2 textureCoordinates = subpassLoad(gBufferTextureCoordinate).xy;
#ifdef PARALLAX_MAPPING
    const vec3 tangentSpaceViewPosition = TBN * viewData.position;
    const vec3 tangentSpaceFragmentPosition = TBN * worldSpacePosition.xyz;
    const vec3 tangentSpaceViewDirection = normalize(tangentSpaceViewPosition - tangentSpaceFragmentPosition);
#if PARALLAX_MAPPING == 0
    textureCoordinates = parallaxMapping(tangentSpaceViewDirection, textureCoordinates, displacement, 0.05);
#elif PARALLAX_MAPPING == 1
    textureCoordinates = parallaxMapping(tangentSpaceViewDirection, textureCoordinates, displacement, 0.15, 8, 32);
#endif
#endif

    /**************************************************************************************************************
     * Compute the normal by combining the geometry normal from the G-Buffer with the data in the normal texture. *
     **************************************************************************************************************/
    const vec3 normal = TBN * normalize(texture(normal, textureCoordinates).xyz);

    /****************************************************************************
     * Extract the depth value for the corresponding fragment in the shadow map *
     ****************************************************************************/
    vec4 shadowMapPosition = lightData.viewProjectionMatrix * worldSpacePosition;  // Transform the world space position of the fragment to the light's Clip Space.
    shadowMapPosition /= shadowMapPosition.w;  // Transform the fragment's light Clip Space position into the light's NDC Space using perspective division (Not actually needed when using orthographic projection when rendering the shadow map because in that case w is 1).
    float shadowDepth = texture(shadowMap, (shadowMapPosition.xy + 1.0) / 2.0).x;  // Transform the fragment's light NDC Space position into the shadowMap's Image Space. ShadowDepth is in the light's NDC Space.

    /***************************
     * Compute the shadow bias *
     ***************************/
    const vec3 fragmentToLightVector = normalize(lightData.position - worldSpacePosition.xyz);  // Compute a vector from the current fragment to the light in World Space.
    const float cosine_Normal_fragmentToLight = max(dot(normal, fragmentToLightVector), 0);  // Compute the cosine of the angle betweeen the fragment normal and the vector to the light.
    const float bias = maxBias * cosine_Normal_fragmentToLight;  // Multiply by the maxBias get the shadow bias.
    shadowMapPosition.z += bias;  // Apply the bias by adding. Adding brings the depth from the shadow map to be closer to 1, in other words, closer to the light source.

    /*****************************************
     * Compute the lighting on this fragment *
     *****************************************/
    vec3 light = vec3(0);
    if (shadowMapPosition.z > shadowDepth) {  // Both shadowMapPosition.z and shadowDepth are in the light view's NDC and the shadowMapPosition.z has already had the bias applied, so we can directly compare them.
        /*********************************************************
         * The fragment is in the light, so compute the lighting *
         *********************************************************/
        float lightIntensity = cosine_Normal_fragmentToLight;  // Decrease the light intensity with the angle at which it hits the surface.
        light = lightColor * lightIntensity;  // The light value is the light's color times the light's intensity.
    }

    /**********************************************************************************************
     * Compute the final color of this fragment by applying the computed lighting to the surface. *
     **********************************************************************************************/
    renderColor = texture(albedo, textureCoordinates);
    renderColor *= vec4(light + ambientLight, 1); // Add ambient light and multiply by albedo to compute final color.
}