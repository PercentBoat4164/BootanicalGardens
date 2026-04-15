#include "Pipeline.hpp"

#include "../Resources/UniformBuffer.hpp"
#include "Shader.hpp"
#include "src/RenderEngine/GraphicsInstance.hpp"
#include "src/RenderEngine/MeshGroup/Material.hpp"
#include "src/RenderEngine/Pipeline/FragmentProcess.hpp"
#include "src/RenderEngine/Pipeline/VertexProcess.hpp"
#include "src/RenderEngine/RenderPass/RenderPass.hpp"

#include <magic_enum/magic_enum.hpp>
#include <volk/volk.h>

#include <deque>

Pipeline::Pipeline(GraphicsDevice* const device, Material* material) : DescriptorSetRequirer(device), device(device), material(material), bindPoint(VK_PIPELINE_BIND_POINT_GRAPHICS) {}

/**@todo: Only rebake if out-of-date.*/
void Pipeline::bake(const std::shared_ptr<RenderPass>& renderPass, const uint32_t subpassIndex, std::span<VkDescriptorSetLayout> layouts, std::deque<std::tuple<void*, std::function<void(void*)>>>& miscMemoryPool, std::vector<VkGraphicsPipelineCreateInfo>& createInfos, std::vector<VkPipeline*>& pipelines) {
  this->renderPass = renderPass;
  // Create the pipeline layout
  std::vector<VkPushConstantRange> pushConstantRanges = material->computePushConstantRanges();
  const VkPipelineLayoutCreateInfo createInfo {
      .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .pNext                  = nullptr,
      .flags                  = 0,
      .setLayoutCount         = static_cast<uint32_t>(layouts.size()),
      .pSetLayouts            = layouts.data(),
      .pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size()),
      .pPushConstantRanges    = pushConstantRanges.data()
  };
  if (const VkResult result = vkCreatePipelineLayout(device->device, &createInfo, nullptr, &layout); result != VK_SUCCESS) GraphicsInstance::showError(result, "failed to create pipeline layout");

  /**@todo: Improve the memory allocation scheme across this entire function. The number of allocations could be greatly reduced by precomputing the required size, then allocating one memory pool and filling that.*/
  const std::vector shaders = {material->vertexProcess->shader, material->fragmentProcess->shader};
  VkPipelineShaderStageCreateInfo(&stages)[] = *static_cast<VkPipelineShaderStageCreateInfo(*)[]>(std::get<0>(miscMemoryPool.emplace_back(new VkPipelineShaderStageCreateInfo[shaders.size()], [](void* mem){ delete[] static_cast<VkPipelineShaderStageCreateInfo*>(mem); })));
  for (uint32_t i{}; i < shaders.size(); ++i) {
    const Shader* shader = shaders[i];
    stages[i] = VkPipelineShaderStageCreateInfo{
        .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext               = nullptr,
        .flags               = 0,
        .stage               = shader->getStage(),
        .module              = shader->getModule(),
        .pName               = shader->getEntryPoint().data(),
        .pSpecializationInfo = VK_NULL_HANDLE
    };
  }

  const std::vector<VkVertexInputBindingDescription>& localBindingDescription = material->computeVertexBindingDescriptions();
  VkVertexInputBindingDescription(&bindingDescription)[] = *static_cast<VkVertexInputBindingDescription(*)[]>(std::get<0>(miscMemoryPool.emplace_back(new VkVertexInputBindingDescription[localBindingDescription.size()], [](void* mem){ delete[] static_cast<VkVertexInputBindingDescription*>(mem); })));
  std::memcpy(bindingDescription, localBindingDescription.data(), localBindingDescription.size() * sizeof(VkVertexInputBindingDescription));
  const std::vector<VkVertexInputAttributeDescription>& localAttributeDescriptions = material->computeVertexAttributeDescriptions();
  VkVertexInputAttributeDescription(&attributeDescriptions)[] = *static_cast<VkVertexInputAttributeDescription(*)[]>(std::get<0>(miscMemoryPool.emplace_back(new VkVertexInputAttributeDescription[localAttributeDescriptions.size()], [](void* mem){ delete[] static_cast<VkVertexInputAttributeDescription*>(mem); })));
  std::memcpy(attributeDescriptions, localAttributeDescriptions.data(), localAttributeDescriptions.size() * sizeof(VkVertexInputAttributeDescription));
  const VkPipelineVertexInputStateCreateInfo& vertexInputState = *static_cast<VkPipelineVertexInputStateCreateInfo*>(std::get<0>(miscMemoryPool.emplace_back(new VkPipelineVertexInputStateCreateInfo{
      .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .pNext                           = nullptr,
      .flags                           = 0,
      .vertexBindingDescriptionCount   = static_cast<uint32_t>(localBindingDescription.size()),
      .pVertexBindingDescriptions      = bindingDescription,
      .vertexAttributeDescriptionCount = static_cast<uint32_t>(localAttributeDescriptions.size()),
      .pVertexAttributeDescriptions    = attributeDescriptions
  }, [](void* mem){ delete static_cast<VkPipelineVertexInputStateCreateInfo*>(mem);})));
  const VkPipelineInputAssemblyStateCreateInfo& inputAssemblyState = *static_cast<VkPipelineInputAssemblyStateCreateInfo*>(std::get<0>(miscMemoryPool.emplace_back(new VkPipelineInputAssemblyStateCreateInfo{
      .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .pNext                  = nullptr,
      .flags                  = 0,
      .topology               = material->vertexProcess->topology,
      .primitiveRestartEnable = material->vertexProcess->primitiveRestartEnable,
  }, [](void* mem){ delete static_cast<VkPipelineInputAssemblyStateCreateInfo*>(mem);})));
  const VkViewport& viewport = *static_cast<VkViewport*>(std::get<0>(miscMemoryPool.emplace_back(new VkViewport{
      .x        = 0,
      .y        = 0,
      .width    = 1,
      .height   = 1,
      .minDepth = 0,
      .maxDepth = 1
  }, [](void* mem){ delete static_cast<VkViewport*>(mem);})));
  const VkRect2D& scissor = *static_cast<VkRect2D*>(std::get<0>(miscMemoryPool.emplace_back(new VkRect2D{
      .offset = {
          .x = 0,
          .y = 0
      },
      .extent = {
          .width = 1,
          .height = 1
      }
  }, [](void* mem){ delete static_cast<VkRect2D*>(mem);})));
  const VkPipelineViewportStateCreateInfo& viewPortState = *static_cast<VkPipelineViewportStateCreateInfo*>(std::get<0>(miscMemoryPool.emplace_back(new VkPipelineViewportStateCreateInfo{
      .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .pNext         = nullptr,
      .flags         = 0,
      .viewportCount = 1,
      .pViewports    = &viewport,
      .scissorCount  = 1,
      .pScissors     = &scissor
  }, [](void* mem){ delete static_cast<VkPipelineViewportStateCreateInfo*>(mem);})));
  const VkPipelineRasterizationStateCreateInfo& rasterizationState = *static_cast<VkPipelineRasterizationStateCreateInfo*>(std::get<0>(miscMemoryPool.emplace_back(new VkPipelineRasterizationStateCreateInfo{
      .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .pNext                   = nullptr,
      .flags                   = 0,
      .depthClampEnable        = material->fragmentProcess->depthState.depthClampEnable,
      .rasterizerDiscardEnable = material->fragmentProcess->rasterizerDiscardEnable,
      .polygonMode             = material->fragmentProcess->polygonMode,
      .cullMode                = material->vertexProcess->cullMode,
      .frontFace               = material->vertexProcess->frontFace,
      .depthBiasEnable         = material->fragmentProcess->depthState.bias.depthBiasEnable,
      .depthBiasConstantFactor = material->fragmentProcess->depthState.bias.depthBiasConstantFactor,
      .depthBiasClamp          = material->fragmentProcess->depthState.bias.depthBiasClamp,
      .depthBiasSlopeFactor    = material->fragmentProcess->depthState.bias.depthBiasSlopeFactor,
      .lineWidth               = material->fragmentProcess->lineWidth
  }, [](void* mem){ delete static_cast<VkPipelineRasterizationStateCreateInfo*>(mem);})));
  const VkPipelineMultisampleStateCreateInfo& multisampleState = *static_cast<VkPipelineMultisampleStateCreateInfo*>(std::get<0>(miscMemoryPool.emplace_back(new VkPipelineMultisampleStateCreateInfo{
      .sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .pNext                 = nullptr,
      .flags                 = 0,
      .rasterizationSamples  = material->fragmentProcess->multisampleState.sampleCount,
      .sampleShadingEnable   = material->fragmentProcess->multisampleState.sampleShading,
      .minSampleShading      = material->fragmentProcess->multisampleState.minSampleShading,
      .pSampleMask           = material->fragmentProcess->multisampleState.pSampleMask,
      .alphaToCoverageEnable = material->fragmentProcess->multisampleState.alphaToCoverageEnable,
      .alphaToOneEnable      = material->fragmentProcess->multisampleState.alphaToOneEnable
  }, [](void* mem){delete static_cast<VkPipelineMultisampleStateCreateInfo*>(mem);})));
  const VkPipelineDepthStencilStateCreateInfo& depthStencilState = *static_cast<VkPipelineDepthStencilStateCreateInfo*>(std::get<0>(miscMemoryPool.emplace_back(new VkPipelineDepthStencilStateCreateInfo{
      .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .pNext                 = nullptr,
      .flags                 = 0,
      .depthTestEnable       = material->fragmentProcess->depthState.test.depthTestEnable,
      .depthWriteEnable      = material->fragmentProcess->depthState.depthWriteEnable,
      .depthCompareOp        = material->fragmentProcess->depthState.test.depthCompareOp,
      .depthBoundsTestEnable = material->fragmentProcess->depthState.test.depthBoundsTestEnable,
      .stencilTestEnable     = material->fragmentProcess->stencilState.stencilTestEnable,
      .front                 = material->fragmentProcess->stencilState.front,
      .back                  = material->fragmentProcess->stencilState.back,
      .minDepthBounds        = material->fragmentProcess->depthState.test.minDepthBounds,
      .maxDepthBounds        = material->fragmentProcess->depthState.test.maxDepthBounds
  }, [](void* mem){ delete static_cast<VkPipelineDepthStencilStateCreateInfo*>(mem);})));
  const std::uint32_t blendAttachmentStateCount = renderPass->subpassData.at(subpassIndex).colorImages.size();
  VkPipelineColorBlendAttachmentState(&blendAttachmentStates)[] = *static_cast<VkPipelineColorBlendAttachmentState(*)[]>(std::get<0>(miscMemoryPool.emplace_back(new VkPipelineColorBlendStateCreateInfo[blendAttachmentStateCount], [](void* mem){ delete[] static_cast<VkPipelineColorBlendStateCreateInfo*>(mem); })));
  for (std::uint32_t i = 0; i < blendAttachmentStateCount; i++)
    blendAttachmentStates[i] = material->fragmentProcess->blendState.blendStates[0];
  const VkPipelineColorBlendStateCreateInfo& colorBlendState = *static_cast<VkPipelineColorBlendStateCreateInfo*>(std::get<0>(miscMemoryPool.emplace_back(new VkPipelineColorBlendStateCreateInfo{
      .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .pNext           = nullptr,
      .flags           = 0,
      .logicOpEnable   = material->fragmentProcess->blendState.logicOpEnable,
      .logicOp         = material->fragmentProcess->blendState.logicOp,
      .attachmentCount = blendAttachmentStateCount,
      .pAttachments    = blendAttachmentStates,
      .blendConstants  = {
        material->fragmentProcess->blendState.blendConstants[0],
        material->fragmentProcess->blendState.blendConstants[1],
        material->fragmentProcess->blendState.blendConstants[2],
        material->fragmentProcess->blendState.blendConstants[3]
      },
  }, [](void* mem){ delete static_cast<VkPipelineColorBlendStateCreateInfo*>(mem);})));
  const VkDynamicState(&dynamicStates)[] = *static_cast<VkDynamicState(*)[]>(std::get<0>(miscMemoryPool.emplace_back(new VkDynamicState[]{
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR
  }, [](void* mem){ delete[] static_cast<VkDynamicState*>(mem);})));
  const VkPipelineDynamicStateCreateInfo& dynamicState = *static_cast<VkPipelineDynamicStateCreateInfo*>(std::get<0>(miscMemoryPool.emplace_back(new VkPipelineDynamicStateCreateInfo{
      .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .pNext             = nullptr,
      .flags             = 0,
      .dynamicStateCount = 2,
      .pDynamicStates    = dynamicStates
  }, [](void* mem){ delete static_cast<VkPipelineDynamicStateCreateInfo*>(mem);})));
  createInfos.push_back(VkGraphicsPipelineCreateInfo{
      .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .pNext               = nullptr,
      .flags               = 0,
      .stageCount          = static_cast<uint32_t>(shaders.size()),
      .pStages             = stages,
      .pVertexInputState   = &vertexInputState,
      .pInputAssemblyState = &inputAssemblyState,
      .pTessellationState  = nullptr,
      .pViewportState      = &viewPortState,
      .pRasterizationState = &rasterizationState,
      .pMultisampleState   = &multisampleState,
      .pDepthStencilState  = &depthStencilState,
      .pColorBlendState    = &colorBlendState,
      .pDynamicState       = &dynamicState,
      .layout              = layout,
      .renderPass          = renderPass->getRenderPass(),
      .subpass             = subpassIndex,
      .basePipelineHandle  = VK_NULL_HANDLE,
      .basePipelineIndex   = -1,
  });
  pipelines.push_back(&pipeline);
}

void Pipeline::writeDescriptorSets(std::deque<std::tuple<void*, std::function<void(void*)>>>& miscMemoryPool, std::vector<VkWriteDescriptorSet>& writes) {
  const std::unordered_map<uint32_t, Material::Binding>* bindings = material->getBindings(2);
  std::unordered_map<uint32_t, std::variant<std::monostate, std::vector<VkDescriptorImageInfo>, std::vector<VkDescriptorBufferInfo>, std::vector<VkBufferView>>> descriptorInfos;

  for (auto& [binding, info] : *bindings) {
    std::variant<std::monostate, std::vector<VkDescriptorImageInfo>, std::vector<VkDescriptorBufferInfo>, std::vector<VkBufferView>>& data = descriptorInfos[binding];
    switch (info.type) {
      case VK_DESCRIPTOR_TYPE_SAMPLER:
      case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
      case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
      case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
      case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
      case VK_DESCRIPTOR_TYPE_SAMPLE_WEIGHT_IMAGE_QCOM:
      case VK_DESCRIPTOR_TYPE_BLOCK_MATCH_IMAGE_QCOM:
        if (std::holds_alternative<std::monostate>(data))
          data.emplace<std::vector<VkDescriptorImageInfo>>();
        else if (!std::holds_alternative<std::vector<VkDescriptorImageInfo>>(data))
          GraphicsInstance::showError("Pipeline::writeDescriptorSets: Not all descriptor types matched when writing descriptors for a single binding!");
        renderPass->bind(std::get<std::vector<VkDescriptorImageInfo>>(data).emplace_back(), this, material, info);
        break;
      case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
      case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
        if (std::holds_alternative<std::monostate>(data))
          data.emplace<std::vector<VkBufferView>>();
        else if (!std::holds_alternative<std::vector<VkBufferView>>(data))
          GraphicsInstance::showError("Pipeline::writeDescriptorSets: Not all descriptor types matched when writing descriptors for a single binding!");
        renderPass->bind(std::get<std::vector<VkBufferView>>(data).emplace_back(), this, material, info);
        break;
      case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
      case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
      case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
      case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
        if (std::holds_alternative<std::monostate>(data))
          data.emplace<std::vector<VkDescriptorBufferInfo>>();
        else if (!std::holds_alternative<std::vector<VkDescriptorBufferInfo>>(data))
          GraphicsInstance::showError("Pipeline::writeDescriptorSets: Not all descriptor types matched when writing descriptors for a single binding!");
        renderPass->bind(std::get<std::vector<VkDescriptorBufferInfo>>(data).emplace_back(), this, material, info);
        break;
      case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
      case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV:
      case VK_DESCRIPTOR_TYPE_MUTABLE_EXT:
      case VK_DESCRIPTOR_TYPE_PARTITIONED_ACCELERATION_STRUCTURE_NV:
      case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT:
      case VK_DESCRIPTOR_TYPE_MAX_ENUM:
        GraphicsInstance::showError("Pipeline::writeDescriptorSets: Invalid descriptor type encountered!");
    }
  }

  // Fill in the writes.
  uint32_t offset = writes.size();
  writes.resize(offset + descriptorInfos.size() * descriptorSets.size());
  for (auto& [binding, info] : descriptorInfos) {
    VkWriteDescriptorSet write {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pNext = nullptr,
      .dstBinding = binding,
      .dstArrayElement = 0,
      .descriptorCount = bindings->at(binding).count,
      .descriptorType = bindings->at(binding).type,
      .pImageInfo = nullptr,
      .pBufferInfo = nullptr,
      .pTexelBufferView = nullptr
    };

    // Move the data to the miscMemoryPool before adding a pointer to the appropriate element of this write.
    if (std::holds_alternative<std::vector<VkDescriptorImageInfo>>(info)) {
      auto& data = std::get<std::vector<VkDescriptorImageInfo>>(info);
      VkDescriptorImageInfo(&pInfo)[] = *static_cast<VkDescriptorImageInfo(*)[]>(std::get<0>(miscMemoryPool.emplace_back(new VkDescriptorImageInfo[data.size()], [](void* mem){ delete[] static_cast<VkDescriptorImageInfo*>(mem); })));
      std::memcpy(pInfo, data.data(), data.size() * sizeof(VkDescriptorImageInfo));
      write.pImageInfo = pInfo;
    }
    else if (std::holds_alternative<std::vector<VkDescriptorBufferInfo>>(info)) {
      auto& data = std::get<std::vector<VkDescriptorBufferInfo>>(info);
      VkDescriptorBufferInfo(&pInfo)[] = *static_cast<VkDescriptorBufferInfo(*)[]>(std::get<0>(miscMemoryPool.emplace_back(new VkDescriptorBufferInfo[data.size()], [](void* mem){ delete[] static_cast<VkDescriptorBufferInfo*>(mem); })));
      std::memcpy(pInfo, data.data(), data.size() * sizeof(VkDescriptorBufferInfo));
      write.pBufferInfo = pInfo;
    }
    else if (std::holds_alternative<std::vector<VkBufferView>>(info)) {
      auto& data = std::get<std::vector<VkBufferView>>(info);
      VkBufferView(&pInfo)[] = *static_cast<VkBufferView(*)[]>(std::get<0>(miscMemoryPool.emplace_back(new VkBufferView[data.size()], [](void* mem){ delete[] static_cast<VkBufferView*>(mem); })));
      std::memcpy(pInfo, data.data(), data.size() * sizeof(VkBufferView));
      write.pTexelBufferView = pInfo;
    }

    // Generate a VkWriteDescriptorSet for each descriptor set.
    for (uint64_t i{}; i < descriptorSets.size(); ++i) {
      write.dstSet = getDescriptorSet(i);
      writes[offset++] = write;
    }
  }
}

Pipeline::~Pipeline() {
  vkDestroyPipelineLayout(device->device, layout, nullptr);
  layout = VK_NULL_HANDLE;
  vkDestroyPipeline(device->device, pipeline, nullptr);
  pipeline = VK_NULL_HANDLE;
}

VkPipeline Pipeline::getPipeline() const { return pipeline; }
VkPipelineLayout Pipeline::getLayout() const { return layout; }
Material* Pipeline::getMaterial() const { return material; }