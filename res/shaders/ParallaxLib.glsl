#ifndef PARALLAXLIB_INCLUDE_GUARD
#define PARALLAXLIB_INCLUDE_GUARD

vec3 parallaxMapping(const in vec3 tangentSpaceViewDirection, const in vec2 textureCoordinates, const in sampler2D displacementMap, const in float heightScale) {
    const float height = texture(displacementMap, textureCoordinates).r;
    const float adjustedHeight = height * heightScale;
    const vec2 p = tangentSpaceViewDirection.xy / tangentSpaceViewDirection.z * adjustedHeight;
    return vec3(textureCoordinates - p, length(tangentSpaceViewDirection) * adjustedHeight);
}

vec3 parallaxMapping(const in vec3 tangentSpaceViewDirection, const in vec2 textureCoordinates, const in sampler2D displacementMap, const in float heightScale, const in uint layerCount) {
    const float layerDepth = 1.0 / layerCount;
    const vec2 p = tangentSpaceViewDirection.xy * heightScale;
    const vec2 deltaTexCoords = p / layerCount;

    vec2  currentTexCoords     = textureCoordinates;
    float currentDepthMapValue = texture(displacementMap, currentTexCoords).r;
    float currentLayerDepth = 0.0;
    while (currentLayerDepth < currentDepthMapValue) {
        currentTexCoords -= deltaTexCoords;
        currentDepthMapValue = texture(displacementMap, currentTexCoords).r;
        currentLayerDepth += layerDepth;
    }

    const vec2 prevTexCoords = currentTexCoords + deltaTexCoords;
    const float afterDepth  = currentDepthMapValue - currentLayerDepth;
    const float beforeDepth = texture(displacementMap, prevTexCoords).r - currentLayerDepth + layerDepth;
    const float weight = afterDepth / (afterDepth - beforeDepth);
    return vec3(fma(prevTexCoords, vec2(weight), currentTexCoords * (1.0 - weight)), length(tangentSpaceViewDirection) * ((afterDepth + beforeDepth) / 2.0) * heightScale);
}

vec3 parallaxMapping(const in vec3 tangentSpaceViewDirection, const in vec2 textureCoordinates, const in sampler2D displacementMap, const in float heightScale, const in uint minLayers, const in uint maxLayers) {
    const uint layerCount = uint(round(mix(maxLayers, minLayers, max(dot(vec3(0.0, 0.0, 1.0), tangentSpaceViewDirection), 0.0))));
    return parallaxMapping(tangentSpaceViewDirection, textureCoordinates, displacementMap, heightScale, layerCount);
}
#endif  // PARALLAXLIB_INCLUDE_GUARD