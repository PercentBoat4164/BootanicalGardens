#version 460

#include "../../BooLib.glsl"


layout (location = 0) in vec2 inTextureCoordinates;
layout (location = 1) in vec4 inOffset[3];

layout (set=PER_MATERIAL_SET, binding=0) uniform sampler2D renderColor;

layout (constant_id = 0) const float SMAA_Threshold = 0.05;
layout (constant_id = 1) const float SMAA_LocalContrastAdaptationFactor = 2.0;

layout (location = 0) out vec2 SMAAEdges;

void main() {
    const vec2 threshold = vec2(SMAA_Threshold, SMAA_Threshold);

    // Calculate lumas
    const vec3 weights = vec3(0.2126, 0.7152, 0.0722);
    float L = dot(pow(texture(renderColor, inTextureCoordinates).rgb, vec3(1.0/2.2)), weights);

    float Lleft = dot(pow(texture(renderColor, inOffset[0].xy).rgb, vec3(1.0/2.2)), weights);
    float Ltop  = dot(pow(texture(renderColor, inOffset[0].zw).rgb, vec3(1.0/2.2)), weights);

    // We do the usual theshold
    vec4 delta;
    delta.xy = abs(L - vec2(Lleft, Ltop));
    SMAAEdges = step(threshold, delta.xy);

    // Then discard if there is no edge
    if (dot(SMAAEdges, vec2(1.0, 1.0)) == 0.0) discard;

    // Calculate right and bottom deltas
    float Lright  = dot(pow(texture(renderColor, inOffset[1].xy).rgb, vec3(1.0/2.2)), weights);
    float Lbottom = dot(pow(texture(renderColor, inOffset[1].zw).rgb, vec3(1.0/2.2)), weights);
    delta.zw = abs(L - vec2(Lright, Lbottom));

    // Calculate the maximum delta in the direct neighborhood
    vec2 maxDelta = max(delta.xy, delta.zw);

    // Calculate left-left and top-top deltas
    float Lleftleft  = dot(pow(texture(renderColor, inOffset[2].xy).rgb, vec3(1.0/2.2)), weights);
    float Ltoptop = dot(pow(texture(renderColor, inOffset[2].zw).rgb, vec3(1.0/2.2)), weights);
    delta.zw = abs(vec2(Lleft, Ltop) - vec2(Lleftleft, Ltoptop));

    // Calculate the final maximum delta
    maxDelta = max(maxDelta.xy, delta.zw);
    float finalDelta = max(maxDelta.x, maxDelta.y);

    // Local contrast adaptation
    SMAAEdges *= step(finalDelta, SMAA_LocalContrastAdaptationFactor * delta.xy);
}