/*************************************
 * Per frame descriptor set bindings *
 *************************************/

/// The number of frames rendered before this frame. The first frame has a frame number of '0'.
#define PER_FRAME_FRAME_NUMBER set=0, binding=0
/// The time the game has been running for in seconds.
#define PER_FRAME_GAME_TIME set=0, binding=1

/******************************************
 * Per RenderPass descriptor set bindings *
 ******************************************/

#define PER_RENDERPASS_DATA layout (set=1, binding=0) uniform PassData {  \
    /**This RenderPass' view projection matrix.*/                         \
    mat4 viewProjectionMatrix;                                            \
}
// @todo: This is probably where lighting data should go.
#define PER_RENDERPASS_CUSTOM_DATA_SET_AND_BINDING set=1, binding=1

/****************************************
 * Per Material descriptor set bindings *
 ****************************************/

/// This material's color texture.
#define PER_MATERIAL_DATA_ALBEDO layout (set=2, binding=0) uniform sampler2D
#define PER_MATERIAL_DATA_NORMAL layout (set=2, binding=1) uniform sampler2D

/******************************************
 * Per Mesh descriptor set bindings *
 ******************************************/

/// This mesh's model to world matrix.
#define PER_MESH_MODEL_DATA layout (set=3, binding=0) uniform MeshData {  \
    mat4 modelMatrix;                                                     \
}