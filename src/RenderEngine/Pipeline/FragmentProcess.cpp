#include "FragmentProcess.hpp"

#include "src/RenderEngine/GraphicsDevice.hpp"

#include <magic_enum/magic_enum.hpp>
#include <yyjson.h>

FragmentProcess FragmentProcess::jsonGet(GraphicsDevice* device, yyjson_val* jsonData) {
  static float id = 0;
  FragmentProcess fragmentProcess;
  fragmentProcess.id = id;
  id += 1.0 / static_cast<double>((1UL << 32U) - 1U);
  fragmentProcess.blendState.blendStates.emplace_back();
  std::uint64_t uintValue;
  const char* enumValue;
  double doubleValue;
  bool boolValue;
  yyjson_ptr_get_uint(jsonData, "/shader", &uintValue);
  fragmentProcess.shader = device->getJSONShader(uintValue);
  yyjson_ptr_get_str(jsonData, "/polygonMode", &enumValue);
  fragmentProcess.polygonMode = magic_enum::enum_cast<VkPolygonMode>(enumValue).value_or(VK_POLYGON_MODE_FILL);
  yyjson_ptr_get_real(jsonData, "/lineWidth", &doubleValue);
  fragmentProcess.lineWidth = static_cast<float>(doubleValue);
  yyjson_ptr_get_bool(jsonData, "/rasterizerDiscardEnable", &boolValue);
  fragmentProcess.rasterizerDiscardEnable = boolValue;
  yyjson_ptr_get_str(jsonData, "/multisampleState/sampleCount", &enumValue);
  fragmentProcess.multisampleState.sampleCount = magic_enum::enum_cast<VkSampleCountFlagBits>(enumValue).value_or(VK_SAMPLE_COUNT_1_BIT);
  yyjson_ptr_get_real(jsonData, "/multisampleState/minSampleShading", &doubleValue);
  fragmentProcess.multisampleState.minSampleShading = static_cast<float>(doubleValue);
  yyjson_ptr_get_bool(jsonData, "/multisampleState/enableSampleMask", &boolValue);
  fragmentProcess.multisampleState.pSampleMask = boolValue ? &fragmentProcess.multisampleState.sampleMask : nullptr;
  yyjson_ptr_get_uint(jsonData, "/multisampleState/sampleMask", &uintValue);
  fragmentProcess.multisampleState.sampleMask = uintValue;
  yyjson_ptr_get_bool(jsonData, "/multisampleState/alphaToCoverageEnable", &boolValue);
  fragmentProcess.multisampleState.alphaToCoverageEnable = boolValue;
  yyjson_ptr_get_bool(jsonData, "/multisampleState/alphaToOneEnable", &boolValue);
  fragmentProcess.multisampleState.alphaToOneEnable = boolValue;
  yyjson_ptr_get_bool(jsonData, "/depthState/depthWriteEnable", &boolValue);
  fragmentProcess.depthState.depthWriteEnable = boolValue;
  yyjson_ptr_get_bool(jsonData, "/depthState/bias/depthBiasEnable", &boolValue);
  fragmentProcess.depthState.bias.depthBiasEnable = boolValue;
  yyjson_ptr_get_bool(jsonData, "/depthState/bias/depthBiasConstantFactor", &boolValue);
  fragmentProcess.depthState.bias.depthBiasConstantFactor = boolValue;
  yyjson_ptr_get_bool(jsonData, "/depthState/bias/depthBiasClamp", &boolValue);
  fragmentProcess.depthState.bias.depthBiasClamp = boolValue;
  yyjson_ptr_get_bool(jsonData, "/depthState/bias/depthBiasSlopeFactor", &boolValue);
  fragmentProcess.depthState.bias.depthBiasClamp = boolValue;
  yyjson_ptr_get_bool(jsonData, "/depthState/test/depthTestEnable", &boolValue);
  fragmentProcess.depthState.test.depthTestEnable = boolValue;
  yyjson_ptr_get_str(jsonData, "/depthState/test/depthCompareOp", &enumValue);
  fragmentProcess.depthState.test.depthCompareOp = magic_enum::enum_cast<VkCompareOp>(enumValue).value_or(VK_COMPARE_OP_LESS);
  yyjson_ptr_get_bool(jsonData, "/depthState/test/depthBoundsTestEnable", &boolValue);
  fragmentProcess.depthState.test.depthBoundsTestEnable = boolValue;
  yyjson_ptr_get_real(jsonData, "/depthState/test/minDepthBounds", &doubleValue);
  fragmentProcess.depthState.test.minDepthBounds = static_cast<float>(doubleValue);
  yyjson_ptr_get_real(jsonData, "/depthState/test/maxDepthBounds", &doubleValue);
  fragmentProcess.depthState.test.maxDepthBounds = static_cast<float>(doubleValue);
  yyjson_ptr_get_bool(jsonData, "/stencilState/stencilTestEnable", &boolValue);
  fragmentProcess.stencilState.stencilTestEnable = boolValue;
  // yyjson_ptr_get_str(jsonData, "/stencilState/front", &enumValue);
  fragmentProcess.stencilState.front = {};
  // yyjson_ptr_get_str(jsonData, "/stencilState/back", &enumValue);
  fragmentProcess.stencilState.back = {};
  yyjson_ptr_get_bool(jsonData, "/blendState/blendStates/0/blendEnable", &boolValue);
  fragmentProcess.blendState.blendStates[0].blendEnable = boolValue;
  yyjson_ptr_get_str(jsonData, "/blendState/blendStates/0/srcColorBlendFactor", &enumValue);
  fragmentProcess.blendState.blendStates[0].srcColorBlendFactor = magic_enum::enum_cast<VkBlendFactor>(enumValue).value_or(VK_BLEND_FACTOR_ONE);
  yyjson_ptr_get_str(jsonData, "/blendState/blendStates/0/dstColorBlendFactor", &enumValue);
  fragmentProcess.blendState.blendStates[0].dstColorBlendFactor = magic_enum::enum_cast<VkBlendFactor>(enumValue).value_or(VK_BLEND_FACTOR_ZERO);
  yyjson_ptr_get_str(jsonData, "/blendState/blendStates/0/colorBlendOp", &enumValue);
  fragmentProcess.blendState.blendStates[0].colorBlendOp = magic_enum::enum_cast<VkBlendOp>(enumValue).value_or(VK_BLEND_OP_ADD);
  yyjson_ptr_get_str(jsonData, "/blendState/blendStates/0/srcAlphaBlendFactor", &enumValue);
  fragmentProcess.blendState.blendStates[0].srcAlphaBlendFactor = magic_enum::enum_cast<VkBlendFactor>(enumValue).value_or(VK_BLEND_FACTOR_ONE);
  yyjson_ptr_get_str(jsonData, "/blendState/blendStates/0/dstAlphaBlendFactor", &enumValue);
  fragmentProcess.blendState.blendStates[0].dstAlphaBlendFactor = magic_enum::enum_cast<VkBlendFactor>(enumValue).value_or(VK_BLEND_FACTOR_ZERO);
  yyjson_ptr_get_str(jsonData, "/blendState/blendStates/0/alphaBlendOp", &enumValue);
  fragmentProcess.blendState.blendStates[0].alphaBlendOp = magic_enum::enum_cast<VkBlendOp>(enumValue).value_or(VK_BLEND_OP_ADD);
  yyjson_ptr_get_uint(jsonData, "/blendState/blendStates/0/colorWriteMask", &uintValue);
  fragmentProcess.blendState.blendStates[0].colorWriteMask = uintValue;
  yyjson_ptr_get_bool(jsonData, "/blendState/logicOpEnable", &boolValue);
  fragmentProcess.blendState.logicOpEnable = boolValue;
  yyjson_ptr_get_str(jsonData, "/blendState/logicOp", &enumValue);
  fragmentProcess.blendState.logicOp = magic_enum::enum_cast<VkLogicOp>(enumValue).value_or(VK_LOGIC_OP_COPY);
  yyjson_ptr_get_real(jsonData, "/blendState/blendConstants/0", &doubleValue);
  fragmentProcess.blendState.blendConstants[0] = static_cast<float>(doubleValue);
  yyjson_ptr_get_real(jsonData, "/blendState/blendConstants/1", &doubleValue);
  fragmentProcess.blendState.blendConstants[1] = static_cast<float>(doubleValue);
  yyjson_ptr_get_real(jsonData, "/blendState/blendConstants/2", &doubleValue);
  fragmentProcess.blendState.blendConstants[2] = static_cast<float>(doubleValue);
  yyjson_ptr_get_real(jsonData, "/blendState/blendConstants/3", &doubleValue);
  fragmentProcess.blendState.blendConstants[3] = static_cast<float>(doubleValue);
  return fragmentProcess;
}