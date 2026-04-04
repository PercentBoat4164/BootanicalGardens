#version 460

#include "../../BooLib.glsl"

layout (location = 0) in vec2 inTextureCoordinates;
layout (location = 1) in vec2 inPixelCoordinates;
layout (location = 2) in vec4 inOffset[3];

layout (set=PER_PASS_SET, binding=0) uniform Resolution {
    vec4 resolution;
};

layout (set=PER_MATERIAL_SET, binding=0) uniform sampler2D SMAAEdges;
layout (set=PER_MATERIAL_SET, binding=1) uniform sampler2D SMAAAreaTexture;
layout (set=PER_MATERIAL_SET, binding=2) uniform sampler2D SMAASearchTexture;

layout (constant_id = 2) const uint SMAA_MaxSearchStepsDiag = 16;
layout (constant_id = 3) const float SMAA_CornerRounding = 0.25;

layout (location = 0) out vec4 SMAABlend;

const vec4 subsampleIndices = vec4(0);


/**
 * Conditional move:
 */
void cmov(bvec2 cond, inout vec2 variable, vec2 value) {
    if (cond.x) variable.x = value.x;
    if (cond.y) variable.y = value.y;
}

/**
 * Allows to decode two binary values from a bilinear-filtered access.
 */
vec2 decodeDiagBilinearAccess(vec2 e) {
    // Bilinear access for fetching 'e' have a 0.25 offset, and we are
    // interested in the R and G edges:
    //
    // +---G---+-------+
    // |   x o R   x   |
    // +-------+-------+
    //
    // Then, if one of these edge is enabled:
    //   Red:   (0.75 * X + 0.25 * 1) => 0.25 or 1.0
    //   Green: (0.75 * 1 + 0.25 * X) => 0.75 or 1.0
    //
    // This function will unpack the values (fma + mul + round):
    // wolframalpha.com: round(x * abs(5 * x - 5 * 0.75)) plot 0 to 1
    e.r = e.r * abs(5.0 * e.r - 5.0 * 0.75);
    return round(e);
}

vec4 decodeDiagBilinearAccess(vec4 e) {
    e.rb = e.rb * abs(5.0 * e.rb - 5.0 * 0.75);
    return round(e);
}

/**
 * These functions allows to perform diagonal pattern searches.
 */
vec2 searchDiag1(sampler2D edgesTex, vec2 texcoord, vec2 dir, out vec2 e) {
    vec4 coord = vec4(texcoord, -1.0, 1.0);
    vec3 t = vec3(resolution.xy, 1.0);
    while (coord.z < float(SMAA_MaxSearchStepsDiag - 1) &&
           coord.w > 0.9) {
        coord.xyz = fma(t, vec3(dir, 1.0), coord.xyz);
        e = textureLod(edgesTex, coord.xy, 0).rg;
        coord.w = dot(e, vec2(0.5, 0.5));
    }
    return coord.zw;
}

vec2 searchDiag2(sampler2D edgesTex, vec2 texcoord, vec2 dir, out vec2 e) {
    vec4 coord = vec4(texcoord, -1.0, 1.0);
    coord.x += 0.25 * resolution.x; // See @SearchDiag2Optimization
    vec3 t = vec3(resolution.xy, 1.0);
    while (coord.z < float(SMAA_MaxSearchStepsDiag - 1) &&
           coord.w > 0.9) {
        coord.xyz = fma(t, vec3(dir, 1.0), coord.xyz);

        // @SearchDiag2Optimization
        // Fetch both edges at once using bilinear filtering:
        e = textureLod(edgesTex, coord.xy, 0).rg;
        e = decodeDiagBilinearAccess(e);

        // Non-optimized version:
        // e.g = textureLod(edgesTex, coord.xy, 0).g;
        // e.r = textureLodOffset(edgesTex, coord.xy, 0, int2(1, 0)).r;

        coord.w = dot(e, vec2(0.5, 0.5));
    }
    return coord.zw;
}

/** 
 * Similar to SMAAArea, this calculates the area corresponding to a certain
 * diagonal distance and crossing edges 'e'.
 */
const uint maxDistanceDiag = 20;
const vec2 maxDistanceDiagVec = vec2(maxDistanceDiag);
const vec2 areaTexPixelSize = 1.0 / vec2(160.0, 560.0);
const float areaTexSubtexSize = 1.0 / 7.0;
vec2 areaDiag(sampler2D areaTex, vec2 dist, vec2 e, float offset) {
    vec2 texcoord = fma(maxDistanceDiagVec, e, dist);

    // We do a scale and bias for mapping to texel space:
    texcoord = fma(areaTexPixelSize, texcoord, 0.5 * areaTexPixelSize);

    // Diagonal areas are on the second half of the texture:
    texcoord.x += 0.5;

    // Move to proper place, according to the subpixel offset:
    texcoord.y += areaTexSubtexSize * offset;

    // Do it!
    return textureLod(areaTex, texcoord.rg, 0).xy;
}

/**
 * This searches for diagonal patterns and returns the corresponding weights.
 */
vec2 calculateDiagWeights(sampler2D edgesTex, sampler2D areaTex, vec2 texcoord, vec2 e, vec4 subsampleIndices) {
    vec2 weights = vec2(0.0, 0.0);

    // Search for the line ends:
    vec4 d;
    vec2 end;
    if (e.r > 0.0) {
        d.xz = searchDiag1(edgesTex, texcoord, vec2(-1.0,  1.0), end);
        d.x += float(end.y > 0.9);
    } else
        d.xz = vec2(0.0, 0.0);
    d.yw = searchDiag1(edgesTex, texcoord, vec2(1.0, -1.0), end);

    if (d.x + d.y > 2.0) { // d.x + d.y + 1 > 3
        // Fetch the crossing edges:
        vec4 coords = fma(vec4(-d.x + 0.25, d.x, d.y, -d.y - 0.25), resolution.xyxy, texcoord.xyxy);
        vec4 c;
        c.xy = textureLodOffset(edgesTex, coords.xy, 0, ivec2(-1,  0)).rg;
        c.zw = textureLodOffset(edgesTex, coords.zw, 0, ivec2( 1,  0)).rg;
        c.yxwz = decodeDiagBilinearAccess(c.xyzw);

        // Non-optimized version:
        // vec4 coords = fma(vec4(-d.x, d.x, d.y, -d.y), resolution.xyxy, texcoord.xyxy);
        // vec4 c;
        // c.x = textureLodOffset(edgesTex, coords.xy, int2(-1,  0)).g;
        // c.y = textureLodOffset(edgesTex, coords.xy, int2( 0,  0)).r;
        // c.z = textureLodOffset(edgesTex, coords.zw, int2( 1,  0)).g;
        // c.w = textureLodOffset(edgesTex, coords.zw, int2( 1, -1)).r;

        // Merge crossing edges at each side into a single value:
        vec2 cc = fma(vec2(2.0, 2.0), c.xz, c.yw);

        // Remove the crossing edge if we didn't found the end of the line:
        cmov(bvec2(step(0.9, d.zw)), cc, vec2(0.0, 0.0));

        // Fetch the areas for this line:
        weights += areaDiag(areaTex, d.xy, cc, subsampleIndices.z);
    }

    // Search for the line ends:
    d.xz = searchDiag2(edgesTex, texcoord, vec2(-1.0, -1.0), end);
    if (textureLodOffset(edgesTex, texcoord, 0, ivec2(1, 0)).r > 0.0) {
        d.yw = searchDiag2(edgesTex, texcoord, vec2(1.0, 1.0), end);
        d.y += float(end.y > 0.9);
    } else
        d.yw = vec2(0.0, 0.0);

    if (d.x + d.y > 2.0) { // d.x + d.y + 1 > 3
        // Fetch the crossing edges:
        vec4 coords = fma(vec4(-d.x, -d.x, d.y, d.y), resolution.xyxy, texcoord.xyxy);
        vec4 c;
        c.x  = textureLodOffset(edgesTex, coords.xy, 0, ivec2(-1,  0)).g;
        c.y  = textureLodOffset(edgesTex, coords.xy, 0, ivec2( 0, -1)).r;
        c.zw = textureLodOffset(edgesTex, coords.zw, 0, ivec2( 1,  0)).gr;
        vec2 cc = fma(vec2(2.0, 2.0), c.xz, c.yw);

        // Remove the crossing edge if we didn't found the end of the line:
        cmov(bvec2(step(0.9, d.zw)), cc, vec2(0.0, 0.0));

        // Fetch the areas for this line:
        weights += areaDiag(areaTex, d.xy, cc, subsampleIndices.w).gr;
    }

    return weights;
}

//-----------------------------------------------------------------------------
// Horizontal/Vertical Search Functions

/**
 * This allows to determine how much length should we add in the last step
 * of the searches. It takes the bilinearly interpolated edge (see 
 * @PSEUDO_GATHER4), and adds 0, 1 or 2, depending on which edges and
 * crossing edges are active.
 */
const vec2 searchTexSize = vec2(66.0, 33.0);
const vec2 searchTexPackedSize = vec2(64.0, 16.0);
float searchLength(sampler2D searchTex, vec2 e, float offset) {
    // The texture is flipped vertically, with left and right cases taking half
    // of the space horizontally:
    vec2 scale = searchTexSize * vec2(0.5, -1.0);
    vec2 bias = searchTexSize * vec2(offset, 1.0);

    // Scale and bias to access texel centers:
    scale += vec2(-1.0,  1.0);
    bias  += vec2( 0.5, -0.5);

    // Convert from pixel coordinates to texcoords:
    // (We use SMAA_SEARCHTEX_PACKED_SIZE because the texture is cropped)
    scale *= 1.0 / searchTexPackedSize;
    bias *= 1.0 / searchTexPackedSize;

    // Lookup the search texture:
    vec2 coords = fma(scale, e, bias);
    return textureLod(searchTex, coords, 0).r;
}

/**
 * Horizontal/vertical search functions for the 2nd pass.
 */
const float edgeThreshold = 0.8281;  // .91**2
float searchXLeft(sampler2D edgesTex, sampler2D searchTex, vec2 texcoord, float end) {
    /**
     * @PSEUDO_GATHER4
     * This texcoord has been offset by (-0.25, -0.125) in the vertex shader to
     * sample between edge, thus fetching four edges in a row.
     * Sampling with different offsets in each direction allows to disambiguate
     * which edges are active from the four fetched ones.
     */
    vec2 e = vec2(0.0, 1.0);
    while (texcoord.x > end && 
           e.g > edgeThreshold && // Is there some edge not activated?
           e.r == 0.0) { // Or is there a crossing edge that breaks the line?
        e = textureLod(edgesTex, texcoord, 0).rg;
        texcoord = fma(-vec2(2.0, 0.0), resolution.xy, texcoord);
    }

    float offset = fma(-(255.0 / 127.0), searchLength(searchTex, e, 0.0), 3.25);
    return fma(resolution.x, offset, texcoord.x);

    // Non-optimized version:
    // We correct the previous (-0.25, -0.125) offset we applied:
    // texcoord.x += 0.25 * resolution.x;

    // The searches are bias by 1, so adjust the coords accordingly:
    // texcoord.x += resolution.x;

    // Disambiguate the length added by the last step:
    // texcoord.x += 2.0 * resolution.x; // Undo last step
    // texcoord.x -= resolution.x * (255.0 / 127.0) * SMAASearchLength(searchTex, e, 0.0);
    // return fma(resolution.x, offset, texcoord.x);
}

float searchXRight(sampler2D edgesTex, sampler2D searchTex, vec2 texcoord, float end) {
    vec2 e = vec2(0.0, 1.0);
    while (texcoord.x < end && 
           e.g > edgeThreshold && // Is there some edge not activated?
           e.r == 0.0) { // Or is there a crossing edge that breaks the line?
        e = textureLod(edgesTex, texcoord, 0).rg;
        texcoord = fma(vec2(2.0, 0.0), resolution.xy, texcoord);
    }
    float offset = fma(-(255.0 / 127.0), searchLength(searchTex, e, 0.5), 3.25);
    return fma(-resolution.x, offset, texcoord.x);
}

float searchYUp(sampler2D edgesTex, sampler2D searchTex, vec2 texcoord, float end) {
    vec2 e = vec2(1.0, 0.0);
    while (texcoord.y > end &&
           e.r > edgeThreshold && // Is there some edge not activated?
           e.g == 0.0) { // Or is there a crossing edge that breaks the line?
        e = textureLod(edgesTex, texcoord, 0).rg;
        texcoord = fma(-vec2(0.0, 2.0), resolution.xy, texcoord);
    }
    float offset = fma(-(255.0 / 127.0), searchLength(searchTex, e.gr, 0.0), 3.25);
    return fma(resolution.y, offset, texcoord.y);
}

float searchYDown(sampler2D edgesTex, sampler2D searchTex, vec2 texcoord, float end) {
    vec2 e = vec2(1.0, 0.0);
    while (texcoord.y < end &&
           e.r > edgeThreshold && // Is there some edge not activated?
           e.g == 0.0) { // Or is there a crossing edge that breaks the line?
        e = textureLod(edgesTex, texcoord, 0).rg;
        texcoord = fma(vec2(0.0, 2.0), resolution.xy, texcoord);
    }
    float offset = fma(-(255.0 / 127.0), searchLength(searchTex, e.gr, 0.5), 3.25);
    return fma(-resolution.y, offset, texcoord.y);
}

/** 
 * Ok, we have the distance and both crossing edges. So, what are the areas
 * at each side of current edge?
 */
const uint maxDistance = 16;
const vec2 maxDistanceVec = vec2(maxDistance);
vec2 area(sampler2D areaTex, vec2 dist, float e1, float e2, float offset) {
    // Rounding prevents precision errors of bilinear filtering:
    vec2 texcoord = fma(maxDistanceVec, round(4.0 * vec2(e1, e2)), dist);
    
    // We do a scale and bias for mapping to texel space:
    texcoord = fma(areaTexPixelSize, texcoord, 0.5 * areaTexPixelSize);

    // Move to proper place, according to the subpixel offset:
    texcoord.y = fma(areaTexSubtexSize, offset, texcoord.y);

    // Do it!
    return textureLod(areaTex, texcoord.rg, 0).xy;
}

//-----------------------------------------------------------------------------
// Corner Detection Functions
void detectHorizontalCornerPattern(sampler2D edgesTex, inout vec2 weights, vec4 texcoord, vec2 d) {
    #if !defined(SMAA_DISABLE_CORNER_DETECTION)
    vec2 leftRight = step(d.xy, d.yx);
    vec2 rounding = (1.0 - SMAA_CornerRounding) * leftRight;

    rounding /= leftRight.x + leftRight.y; // Reduce blending for pixels in the center of a line.

    vec2 factor = vec2(1.0, 1.0);
    factor.x -= rounding.x * textureLodOffset(edgesTex, texcoord.xy, 0, ivec2(0,  1)).r;
    factor.x -= rounding.y * textureLodOffset(edgesTex, texcoord.zw, 0, ivec2(1,  1)).r;
    factor.y -= rounding.x * textureLodOffset(edgesTex, texcoord.xy, 0, ivec2(0, -2)).r;
    factor.y -= rounding.y * textureLodOffset(edgesTex, texcoord.zw, 0, ivec2(1, -2)).r;

    weights *= clamp(factor, 0, 1);
    #endif
}

void detectVerticalCornerPattern(sampler2D edgesTex, inout vec2 weights, vec4 texcoord, vec2 d) {
    #if !defined(SMAA_DISABLE_CORNER_DETECTION)
    vec2 leftRight = step(d.xy, d.yx);
    vec2 rounding = (1.0 - SMAA_CornerRounding) * leftRight;

    rounding /= leftRight.x + leftRight.y;

    vec2 factor = vec2(1.0, 1.0);
    factor.x -= rounding.x * textureLodOffset(edgesTex, texcoord.xy, 0, ivec2( 1, 0)).g;
    factor.x -= rounding.y * textureLodOffset(edgesTex, texcoord.zw, 0, ivec2( 1, 1)).g;
    factor.y -= rounding.x * textureLodOffset(edgesTex, texcoord.xy, 0, ivec2(-2, 0)).g;
    factor.y -= rounding.y * textureLodOffset(edgesTex, texcoord.zw, 0, ivec2(-2, 1)).g;

    weights *= clamp(factor, 0, 1);
    #endif
}

void main() {
    vec2 e = texture(SMAAEdges, inTextureCoordinates).rg;
    SMAABlend = vec4(0);

    if (e.g > 0.0) { // Edge at north
         #if !defined(SMAA_DISABLE_DIAG_DETECTION)
        // Diagonals have both north and west edges, so searching for them in
        // one of the boundaries is enough.
        SMAABlend.rg = calculateDiagWeights(SMAAEdges, SMAAAreaTexture, inTextureCoordinates, e, subsampleIndices);

        // We give priority to diagonals, so if we find a diagonal we skip
        // horizontal/vertical processing.
        if (SMAABlend.r == -SMAABlend.g) { // weights.r + weights.g == 0.0
        #endif
        
        vec2 d;
        
        // Find the distance to the left:
        vec3 coords;
        coords.x = searchXLeft(SMAAEdges, SMAASearchTexture, inOffset[0].xy, inOffset[2].x);
        coords.y = inOffset[1].y; // inOffset[1].y = inTextureCoordinates.y - 0.25 * resolution.y (@CROSSING_OFFSET)
        d.x = coords.x;
        
        // Now fetch the left crossing edges, two at a time using bilinear
        // filtering. Sampling at -0.25 (see @CROSSING_OFFSET) enables to
        // discern what value each edge has:
        float e1 = textureLod(SMAAEdges, coords.xy, 0).r;
        
        // Find the distance to the right:
        coords.z = searchXRight(SMAAEdges, SMAASearchTexture, inOffset[0].zw, inOffset[2].y);
        d.y = coords.z;
        
        // We want the distances to be in pixel units (doing this here allow to
        // better interleave arithmetic and memory accesses):
        d = abs(round(fma(resolution.zz, d, -inPixelCoordinates.xx)));
        
        // SMAAArea below needs a sqrt, as the areas texture is compressed
        // quadratically:
        vec2 sqrt_d = sqrt(d);

        // Fetch the right crossing edges:
        float e2 = textureLodOffset(SMAAEdges, coords.zy, 0, ivec2(1, 0)).r;
        
        // Ok, we know how this pattern looks like, now it is time for getting
        // the actual area:
        SMAABlend.rg = area(SMAAAreaTexture, sqrt_d, e1, e2, subsampleIndices.y);

        // Fix corners:
        coords.y = inTextureCoordinates.y;
        detectHorizontalCornerPattern(SMAAEdges, SMAABlend.rg, coords.xyzy, d);
            
        #if !defined(SMAA_DISABLE_DIAG_DETECTION)
        } else
            e.r = 0.0; // Skip vertical processing.
        #endif
    }

    if (e.r > 0.0) { // Edge at west
        vec2 d;
        
        // Find the distance to the top:
        vec3 coords;
        coords.y = searchYUp(SMAAEdges, SMAASearchTexture, inOffset[1].xy, inOffset[2].z);
        coords.x = inOffset[0].x; // inOffset[1].x = inTextureCoordinates.x - 0.25 * resolution.x;
        d.x = coords.y;
        
        // Fetch the top crossing edges:
        float e1 = textureLod(SMAAEdges, coords.xy, 0).g;
        
        // Find the distance to the bottom:
        coords.z = searchYDown(SMAAEdges, SMAASearchTexture, inOffset[1].zw, inOffset[2].w);
        d.y = coords.z;
        
        // We want the distances to be in pixel units:
        d = abs(round(fma(resolution.ww, d, -inPixelCoordinates.yy)));
        
        // SMAAArea below needs a sqrt, as the areas texture is compressed 
        // quadratically:
        vec2 sqrt_d = sqrt(d);

        // Fetch the bottom crossing edges:
        float e2 = textureLodOffset(SMAAEdges, coords.xz, 0, ivec2(0, 1)).g;
        
        // Get the area for this direction:
        SMAABlend.ba = area(SMAAAreaTexture, sqrt_d, e1, e2, subsampleIndices.x);
        
        // Fix corners:
        coords.x = inTextureCoordinates.x;
        detectVerticalCornerPattern(SMAAEdges, SMAABlend.ba, coords.xyxz, d);
    }
}