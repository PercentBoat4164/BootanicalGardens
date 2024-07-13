#include "Pipeline.hpp"

#include "GraphicsInstance.hpp"
#include "Shader.hpp"

#include <volk.h>

Pipeline::Pipeline(const GraphicsDevice& device, const std::vector<ShaderUsage*>& shaderUsages) : device(device), shaderUsages(shaderUsages) {
  if (shaderUsages.empty()) GraphicsInstance::showError(false, "A pipeline must be created with at least one shader.");
  bindPoint = shaderUsages.back()->shader.getBindPoint();
  for (const auto* shaderUsage: shaderUsages)
    if (shaderUsage->shader.getBindPoint() != bindPoint) GraphicsInstance::showError(false, "Attempted to create a pipeline of inferred bind point " + std::string(magic_enum::enum_name(bindPoint)) + " using a shader with bind point " + std::string(magic_enum::enum_name(shaderUsage->shader.getBindPoint())) + ".");
  {
    std::vector<VkDescriptorSetLayout> layouts(shaderUsages.size());
    for (uint32_t i{}; i < shaderUsages.size(); ++i)
      layouts[i] = shaderUsages[i]->shader.layout;
    const VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext                  = nullptr,
        .flags                  = 0,
        .setLayoutCount         = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts            = layouts.data(),
        .pushConstantRangeCount = 0,
        .pPushConstantRanges    = nullptr
    };
    GraphicsInstance::showError(vkCreatePipelineLayout(device.device, &pipelineLayoutCreateInfo, nullptr, &layout), "Failed to create pipeline layout.");
  }

  if (bindPoint == VK_PIPELINE_BIND_POINT_COMPUTE) {
    const Shader& shader = shaderUsages.back()->shader;
    const VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo{
        .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext               = nullptr,
        .flags               = 0,
        .stage               = VK_SHADER_STAGE_COMPUTE_BIT,
        .module              = shader.module,
        .pName               = "main",
        .pSpecializationInfo = nullptr
    };
    const VkComputePipelineCreateInfo computePipelineCreateInfo{
        .sType              = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .pNext              = nullptr,
        .flags              = 0,
        .stage              = pipelineShaderStageCreateInfo,
        .layout             = layout,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex  = 0
    };
    GraphicsInstance::showError(vkCreateComputePipelines(device.device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &pipeline), "Failed to create pipeline.");
  }
}

Pipeline::~Pipeline() {
  vkDestroyPipeline(device.device, pipeline, nullptr);
  pipeline = VK_NULL_HANDLE;
  vkDestroyPipelineLayout(device.device, layout, nullptr);
  layout = VK_NULL_HANDLE;
}

void Pipeline::execute(VkCommandBuffer commandBuffer, const uint32_t x, const uint32_t y, const uint32_t z) const {
  std::vector<VkDescriptorSet> descriptorSets(shaderUsages.size(), VK_NULL_HANDLE);
  for (uint32_t i{}; i < shaderUsages.size(); ++i) descriptorSets[i] = shaderUsages[i]->set;
  vkCmdBindPipeline(commandBuffer, bindPoint, pipeline);
  vkCmdBindDescriptorSets(commandBuffer, bindPoint, layout, 0, descriptorSets.size(), descriptorSets.data(), 0, nullptr);
  if (bindPoint == VK_PIPELINE_BIND_POINT_COMPUTE) {
    vkCmdDispatch(commandBuffer, x, y, z);
  }
}