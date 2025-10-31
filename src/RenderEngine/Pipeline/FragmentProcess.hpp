#pragma once

#include <vulkan/vulkan_core.h>

#include <array>
#include <vector>

class GraphicsDevice;
class Shader;

class yyjson_val;

struct FragmentProcess {
  Shader* shader = nullptr;
  float id;

  VkPolygonMode polygonMode         = VK_POLYGON_MODE_FILL;
  float lineWidth                   = 1;
  VkBool32 rasterizerDiscardEnable  = VK_FALSE;

  struct MultisampleState {
    VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;
    VkBool32 sampleShading            = VK_FALSE;
    float minSampleShading            = 1;
    VkSampleMask sampleMask           = 0;
    VkSampleMask* pSampleMask         = nullptr;
    VkBool32 alphaToCoverageEnable    = VK_FALSE;
    VkBool32 alphaToOneEnable         = VK_FALSE;
  } multisampleState;

  struct DepthState {
    VkBool32 depthClampEnable = VK_FALSE;
    VkBool32 depthWriteEnable = VK_TRUE;
    struct Bias {
      VkBool32 depthBiasEnable      = VK_FALSE;
      float depthBiasConstantFactor = 0;
      float depthBiasClamp          = 0;
      float depthBiasSlopeFactor    = 0;
    } bias;
    struct Test {
      VkBool32 depthTestEnable       = VK_TRUE;
      VkCompareOp depthCompareOp     = VK_COMPARE_OP_LESS;
      VkBool32 depthBoundsTestEnable = VK_FALSE;
      float minDepthBounds           = 0;
      float maxDepthBounds           = 1;
    } test;
  } depthState;

  struct StencilState {
    VkBool32 stencilTestEnable = VK_FALSE;
    VkStencilOpState front{};
    VkStencilOpState back{};
  } stencilState;

  struct BlendState {
    std::vector<VkPipelineColorBlendAttachmentState> blendStates{{
      .blendEnable         = VK_FALSE,
      .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
      .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
      .colorBlendOp        = VK_BLEND_OP_ADD,
      .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
      .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
      .alphaBlendOp        = VK_BLEND_OP_ADD,
      .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    }};
    VkBool32 logicOpEnable = VK_FALSE;
    VkLogicOp logicOp      = VK_LOGIC_OP_COPY;
    std::array<float, 4> blendConstants = {0.0f, 0.0f, 0.0f, 0.0f};
  } blendState;

  static FragmentProcess jsonGet(GraphicsDevice* device, yyjson_val* jsonData);
};