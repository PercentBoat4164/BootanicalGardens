#include "Pipeline.hpp"

#include "GraphicsInstance.hpp"
#include "Shader.hpp"

#include <volk.h>

#include <ranges>

Pipeline::Pipeline(const GraphicsDevice& device, const std::vector<ShaderUsage*>& shaderUsages) : device(device), shaderUsages(shaderUsages) {
  if (shaderUsages.empty()) GraphicsInstance::showError(false, "A pipeline must be created with at least one shader.");
  bindPoint = shaderUsages.back()->shader.getBindPoint();
  for (const auto* shaderUsage: shaderUsages)
    if (shaderUsage->shader.getBindPoint() != bindPoint) GraphicsInstance::showError(false, "Attempted to create a pipeline of inferred bind point " + std::string(magic_enum::enum_name(bindPoint)) + " using a shader with bind point " + std::string(magic_enum::enum_name(shaderUsage->shader.getBindPoint())) + ".");
  {
    std::vector<VkDescriptorSetLayout> layouts;
    for (uint32_t i{}; i < shaderUsages.size(); ++i)
      for (const VkDescriptorSetLayout& descriptorLayout: shaderUsages[i]->shader.layouts)
        layouts.push_back(descriptorLayout);
    const VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext                  = nullptr,
        .flags                  = 0,
        .setLayoutCount         = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts            = layouts.data(),
        .pushConstantRangeCount = 0,
        .pPushConstantRanges    = nullptr,
    };
    GraphicsInstance::showError(vkCreatePipelineLayout(device.device, &pipelineLayoutCreateInfo, nullptr, &layout), "Failed to create pipeline layout.");
  }

  if (bindPoint == VK_PIPELINE_BIND_POINT_COMPUTE) {
    const Shader& shader = shaderUsages.back()->shader;
    const VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo{
        .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext               = nullptr,
        .flags               = 0,
        .stage               = shader.stage,
        .module              = shader.module,
        .pName               = shader.entryPoint.c_str(),
        .pSpecializationInfo = nullptr
    };
    const VkComputePipelineCreateInfo pipelineCreateInfo{
        .sType              = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .pNext              = nullptr,
        .flags              = 0,
        .stage              = pipelineShaderStageCreateInfo,
        .layout             = layout,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex  = 0
    };
    GraphicsInstance::showError(vkCreateComputePipelines(device.device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline), "Failed to create compute pipeline.");
  } else {
    std::vector<VkPipelineShaderStageCreateInfo> stages{shaderUsages.size()};
    for (std::tuple zip : std::ranges::views::zip(shaderUsages, stages)) {
      const Shader& shader = std::get<0>(zip)->shader;
      std::get<1>(zip) = VkPipelineShaderStageCreateInfo{
        .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext               = nullptr,
        .flags               = 0,
        .stage               = shader.stage,
        .module              = shader.module,
        .pName               = shader.entryPoint.c_str(),
        .pSpecializationInfo = VK_NULL_HANDLE,
      };
    }

    std::vector<std::vector<VkVertexInputBindingDescription>> vertexBindingDescriptions{shaderUsages.size()};
    std::vector<std::vector<VkVertexInputAttributeDescription>> attributeDescriptions{shaderUsages.size()};
    std::vector<VkPipelineVertexInputStateCreateInfo> vertexInputStates{shaderUsages.size()};
    for (std::tuple zip : std::ranges::views::zip(shaderUsages, vertexBindingDescriptions, attributeDescriptions, vertexInputStates)) {
      const Shader& shader = std::get<0>(zip)->shader;
      // std::get<1>(zip).resize(shader.vertexFormat);
      for (VkVertexInputBindingDescription& vertexBindingDescription : std::get<1>(zip)) {
        vertexBindingDescription.binding = 0;
        vertexBindingDescription.stride = 0;
        vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
      }
      for (VkVertexInputAttributeDescription& attributeDescription : std::get<2>(zip)) {
        attributeDescription.binding  = 0;
        attributeDescription.format   = VK_FORMAT_R32G32_SFLOAT;
        attributeDescription.location = 0;
        attributeDescription.offset   = 0;
      }
      std::get<3>(zip) = VkPipelineVertexInputStateCreateInfo{
        .sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext                           = nullptr,
        .flags                           = 0,
        .vertexBindingDescriptionCount   = static_cast<uint32_t>(std::get<1>(zip).size()),
        .pVertexBindingDescriptions      = std::get<1>(zip).data(),
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(std::get<2>(zip).size()),
        .pVertexAttributeDescriptions    = std::get<2>(zip).data(),
      };
    }

    const VkGraphicsPipelineCreateInfo pipelineCreateInfo {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .stageCount = static_cast<uint32_t>(stages.size()),
      .pStages = stages.data(),
      .pVertexInputState = vertexInputStates.data(),
      // .pInputAssemblyState = inputAssemblyStates.data()
//      .pInputAssemblyState = ,
//      .pTessellationState = ,
//      .pViewportState = ,
//      .pRasterizationState = ,
//      .pMultisampleState = ,
//      .pDepthStencilState = ,
//      .pColorBlendState = ,
//      .pDynamicState = ,
      .layout = layout,
      .renderPass = VK_NULL_HANDLE,
      .subpass = 0,
      .basePipelineHandle = VK_NULL_HANDLE,
      .basePipelineIndex = 0,
    };
    GraphicsInstance::showError(vkCreateGraphicsPipelines(device.device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline), "Failed to create graphics pipeline.");
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