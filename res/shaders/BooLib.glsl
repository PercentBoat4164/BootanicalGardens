/*************************************
 * Per frame descriptor set bindings *
 *************************************/

#define PER_FRAME_SET 0
#define PER_FRAME_DATA layout (set=PER_FRAME_SET, binding=0) uniform FrameData {                      \
    /**The number of frames rendered before this frame. The first frame has a frame number of '0'.*/  \
    uint frameNumber;                                                                                 \
    /**The time the game has been running for in seconds.*/                                           \
    float time;                                                                                       \
}

/******************************************
 * Per RenderPass descriptor set bindings *
 ******************************************/

#define PER_PASS_SET 1
#define PER_PASS_DATA layout (set=PER_PASS_SET, binding=0) uniform PassData {  \
    /**This RenderPass' view projection matrix.*/                              \
    mat4 viewProjectionMatrix;                                                 \
}

// @todo: This is probably where lighting data should go.
#define PER_RENDERPASS_CUSTOM_DATA_SET_AND_BINDING set=PER_PASS_SET, binding=1

/****************************************
 * Per Material descriptor set bindings *
 ****************************************/

#define PER_MATERIAL_SET 2
/// This material's color texture.
#define PER_MATERIAL_DATA_ALBEDO layout (set=PER_MATERIAL_SET, binding = 0) uniform sampler2D
/// This material's normal texture.
#define PER_MATERIAL_DATA_NORMAL layout (set=PER_MATERIAL_SET, binding = 1) uniform sampler2D

/******************************************
 * Per Mesh descriptor set bindings *
 ******************************************/

#define PER_MESH_SET 3
#define PER_MESH_DATA layout (set=PER_MESH_SET, binding=0) uniform MeshData {  \
    /**This mesh's model to world matrix.*/                         \
    mat4 modelMatrix;                                               \
}