/*************************************
 * Per frame descriptor set bindings *
 *************************************/

#define PER_FRAME_SET 0

/******************************************
 * Per RenderPass descriptor set bindings *
 ******************************************/

#define PER_PASS_SET 1

/****************************************
 * Per Material descriptor set bindings *
 ****************************************/

#define PER_MATERIAL_SET 2

#ifdef GL_VERTEX_SHADER
void fullscreenTriangle(float depth) {
    gl_Position = vec4(vec2(gl_VertexIndex & 2, (gl_VertexIndex << 1) & 2) * vec2(2, -2) + vec2(-1, 1), depth, 1);
}

void fullscreenTriangle() {
    fullscreenTriangle(0);
}

void fullscreenTriangle(inout vec2 textureCoordinates, float depth) {
    textureCoordinates = vec2(gl_VertexIndex & 2, (gl_VertexIndex << 1) & 2);
    gl_Position = vec4(textureCoordinates * vec2(2, -2) + vec2(-1, 1), 0, 1);
    textureCoordinates.y = -textureCoordinates.y + 1;
}

void fullscreenTriangle(inout vec2 textureCoordinates) {
    fullscreenTriangle(textureCoordinates, 0);
}
#endif