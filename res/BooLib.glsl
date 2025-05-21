/*************************************
 * Per frame descriptor set bindings *
 *************************************/

/// The number of frames rendered before this frame. The first frame has a frame number of '0'.
#define PER_FRAME_FRAME_NUMBER set=0, binding=0
/// The time the game has been active for in seconds.
#define PER_FRAME_GAME_TIME set=0, binding=1

/******************************************
 * Per RenderPass descriptor set bindings *
 ******************************************/

/// This RenderPass' view projection matrix.
#define PER_RENDERPASS_VIEW_PROJECTION_MATRIX set=1, binding=0
// @todo: This is probably where lighting data should go.

/****************************************
 * Per Material descriptor set bindings *
 ****************************************/

/// This material's color texture.
#define PER_MATERIAL_COLOR_TEXTURE set=2, binding=0
/// This material's normal texture.
#define PER_MATERIAL_NORMAL_TEXTURE set=2, binding=1

/******************************************
 * Per Renderable descriptor set bindings *
 ******************************************/

/// This renderable's model to world matrix.
#define PER_RENDERABLE_MODEL_MATRIX set=3, binding=0