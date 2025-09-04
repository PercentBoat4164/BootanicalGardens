#include "Pipeline.hpp"

#include "Resources/UniformBuffer.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "src/RenderEngine/GraphicsInstance.hpp"
#include "src/RenderEngine/RenderPass/RenderPass.hpp"
#include "src/RenderEngine/Renderable/Material.hpp"
#include "src/RenderEngine/Renderable/Texture.hpp"
#include "src/RenderEngine/Renderable/Vertex.hpp"
#include "src/RenderEngine/Shader.hpp"

#include <magic_enum/magic_enum.hpp>
#include <volk/volk.h>

Pipeline::Pipeline(GraphicsDevice* const device, const std::shared_ptr<Material>& material) : DescriptorSetRequirer(device), device(device), material(material), bindPoint(VK_PIPELINE_BIND_POINT_GRAPHICS) {}

/**@todo: Only rebake if out-of-date.*/
void Pipeline::bake(const std::shared_ptr<const RenderPass>& renderPass, uint32_t subpassIndex, std::span<VkDescriptorSetLayout> layouts, std::vector<void*>& miscMemoryPool, std::vector<VkGraphicsPipelineCreateInfo>& createInfos, std::vector<VkPipeline*>& pipelines) {
  // Create the pipeline layout
  const VkPipelineLayoutCreateInfo createInfo {
      .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .pNext                  = nullptr,
      .flags                  = 0,
      .setLayoutCount         = static_cast<uint32_t>(layouts.size()),
      .pSetLayouts            = layouts.data(),
      .pushConstantRangeCount = 0,
      .pPushConstantRanges    = nullptr
  };
  if (const VkResult result = vkCreatePipelineLayout(device->device, &createInfo, nullptr, &layout); result != VK_SUCCESS) GraphicsInstance::showError(result, "failed to create pipeline layout");

  /**@todo: Improve the memory allocation scheme across this entire function. Allocations could be greatly reduced both in quantity and size by precomputing the required size, then allocating one memory pool and filling that.*/
  const std::vector shaders = {material->getVertexShader(), material->getFragmentShader()};
  std::vector<VkPipelineShaderStageCreateInfo>& stages = *static_cast<std::vector<VkPipelineShaderStageCreateInfo>*>(miscMemoryPool.emplace_back(new std::vector<VkPipelineShaderStageCreateInfo>(shaders.size())));
  for (uint32_t i{}; i < shaders.size(); ++i) {
    const std::shared_ptr<const Shader>& shader = shaders[i];
    stages[i]                                   = VkPipelineShaderStageCreateInfo{
        .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext               = nullptr,
        .flags               = 0,
        .stage               = shader->getStage(),
        .module              = shader->getModule(),
        .pName               = shader->getEntryPoint().data(),
        .pSpecializationInfo = VK_NULL_HANDLE
    };
  }

  /**@todo: Reflect the vertex format.*/
  const VkVertexInputBindingDescription& bindingDescription = *static_cast<VkVertexInputBindingDescription*>(miscMemoryPool.emplace_back(new VkVertexInputBindingDescription{Vertex::getBindingDescription()}));
  const std::vector<VkVertexInputAttributeDescription>& attributeDescriptions = *static_cast<std::vector<VkVertexInputAttributeDescription>*>(miscMemoryPool.emplace_back(new std::vector{std::move(Vertex::getAttributeDescriptions())}));
  const VkPipelineVertexInputStateCreateInfo& vertexInputState = *static_cast<VkPipelineVertexInputStateCreateInfo*>(miscMemoryPool.emplace_back(new VkPipelineVertexInputStateCreateInfo{
      .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .pNext                           = nullptr,
      .flags                           = 0,
      .vertexBindingDescriptionCount   = 1,
      .pVertexBindingDescriptions      = &bindingDescription,
      .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
      .pVertexAttributeDescriptions    = attributeDescriptions.data()
  }));
  const VkPipelineInputAssemblyStateCreateInfo& inputAssemblyState = *static_cast<VkPipelineInputAssemblyStateCreateInfo*>(miscMemoryPool.emplace_back(new VkPipelineInputAssemblyStateCreateInfo{
      .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .pNext                  = nullptr,
      .flags                  = 0,
      .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,  /**@todo: Support multiple topologies. Low priority. Topology is recorded on a per-mesh basis.*/
      .primitiveRestartEnable = VK_FALSE
  }));
  const VkViewport& viewport = *static_cast<VkViewport*>(miscMemoryPool.emplace_back(new VkViewport{
      .x        = 0,
      .y        = 0,
      .width    = 1,
      .height   = 1,
      .minDepth = 0,
      .maxDepth = 1
  }));
  const VkRect2D& scissor = *static_cast<VkRect2D*>(miscMemoryPool.emplace_back(new VkRect2D{
      .offset = {
          .x = 0,
          .y = 0
      },
      .extent = {
          .width = 1,
          .height = 1
      }
  }));
  const VkPipelineViewportStateCreateInfo& viewPortState = *static_cast<VkPipelineViewportStateCreateInfo*>(miscMemoryPool.emplace_back(new VkPipelineViewportStateCreateInfo{
      .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .pNext         = nullptr,
      .flags         = 0,
      .viewportCount = 1,
      .pViewports    = &viewport,
      .scissorCount  = 1,
      .pScissors     = &scissor
  }));
  const VkPipelineRasterizationStateCreateInfo& rasterizationState = *static_cast<VkPipelineRasterizationStateCreateInfo*>(miscMemoryPool.emplace_back(new VkPipelineRasterizationStateCreateInfo{
      .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .pNext                   = nullptr,
      .flags                   = 0,
      .depthClampEnable        = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode             = VK_POLYGON_MODE_FILL,
      .cullMode                = static_cast<VkCullModeFlags>(material->isDoubleSided() ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT),
      .frontFace               = VK_FRONT_FACE_CLOCKWISE,
      .depthBiasEnable         = VK_FALSE,
      .depthBiasConstantFactor = 0.0,
      .depthBiasClamp          = 0.0,
      .depthBiasSlopeFactor    = 0.0,
      .lineWidth               = 1.0
  }));
  const VkPipelineMultisampleStateCreateInfo& multisampleState = *static_cast<VkPipelineMultisampleStateCreateInfo*>(miscMemoryPool.emplace_back(new VkPipelineMultisampleStateCreateInfo{
      .sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .pNext                 = nullptr,
      .flags                 = 0,
      .rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
      .sampleShadingEnable   = VK_FALSE,
      .minSampleShading      = 1.0,
      .pSampleMask           = nullptr,
      .alphaToCoverageEnable = VK_FALSE,
      .alphaToOneEnable      = VK_FALSE
  }));
  const VkPipelineDepthStencilStateCreateInfo& depthStencilState = *static_cast<VkPipelineDepthStencilStateCreateInfo*>(miscMemoryPool.emplace_back(new VkPipelineDepthStencilStateCreateInfo{
      .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .pNext                 = nullptr,
      .flags                 = 0,
      .depthTestEnable       = VK_TRUE,
      .depthWriteEnable      = VK_TRUE,
      .depthCompareOp        = VK_COMPARE_OP_LESS,
      .depthBoundsTestEnable = VK_FALSE,
      .stencilTestEnable     = VK_FALSE,
      .front                 = {
          .failOp      = VK_STENCIL_OP_KEEP,
          .passOp      = VK_STENCIL_OP_KEEP,
          .depthFailOp = VK_STENCIL_OP_KEEP,
          .compareOp   = VK_COMPARE_OP_LESS,
          .compareMask = 0,
          .writeMask   = 0,
          .reference   = 0
      },
      .back           = {.failOp = VK_STENCIL_OP_KEEP, .passOp = VK_STENCIL_OP_KEEP, .depthFailOp = VK_STENCIL_OP_KEEP, .compareOp = VK_COMPARE_OP_LESS, .compareMask = 0, .writeMask = 0, .reference = 0},
      .minDepthBounds = 0.0,
      .maxDepthBounds = 1.0
  }));
  const std::vector<VkPipelineColorBlendAttachmentState>& blendAttachmentStates = *static_cast<std::vector<VkPipelineColorBlendAttachmentState>*>(miscMemoryPool.emplace_back(new std::vector<VkPipelineColorBlendAttachmentState>(renderPass->subpassData.at(subpassIndex).colorImages.size(), {
      .blendEnable         = VK_FALSE,
      .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
      .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
      .colorBlendOp        = VK_BLEND_OP_ADD,
      .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
      .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
      .alphaBlendOp        = VK_BLEND_OP_ADD,
      .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
  })));
  const VkPipelineColorBlendStateCreateInfo& colorBlendState = *static_cast<VkPipelineColorBlendStateCreateInfo*>(miscMemoryPool.emplace_back(new VkPipelineColorBlendStateCreateInfo{
      .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .pNext           = nullptr,
      .flags           = 0,
      .logicOpEnable   = VK_FALSE,
      .logicOp         = VK_LOGIC_OP_COPY,
      .attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size()),
      .pAttachments    = blendAttachmentStates.data(),
      .blendConstants  = {0, 0, 0, 0}
  }));
  const VkDynamicState(&dynamicStates)[] = *static_cast<VkDynamicState(*)[]>(miscMemoryPool.emplace_back(new VkDynamicState[]{
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR
  }));
  const VkPipelineDynamicStateCreateInfo& dynamicState = *static_cast<VkPipelineDynamicStateCreateInfo*>(miscMemoryPool.emplace_back(new VkPipelineDynamicStateCreateInfo{
      .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .pNext             = nullptr,
      .flags             = 0,
      .dynamicStateCount = 2,
      .pDynamicStates    = dynamicStates
  }));
  createInfos.push_back(VkGraphicsPipelineCreateInfo{
      .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .pNext               = nullptr,
      .flags               = 0,
      .stageCount          = static_cast<uint32_t>(stages.size()),
      .pStages             = stages.data(),
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

void Pipeline::writeDescriptorSets(std::vector<void*>& miscMemoryPool, std::vector<VkWriteDescriptorSet>& writes, const RenderGraph& graph) {
  struct MaterialData {
    glm::mat4 light_ViewProjectionMatrix;
  };
  static auto uniformBuffer = std::make_shared<UniformBuffer<MaterialData>>(device, "Light Data");  /**@todo: Store this with the material. This is the cause of the memory leaks right now.*/
  const std::unordered_map<uint32_t, Material::Binding>* bindings = material->getBindings(2);
  std::unordered_map<uint32_t, std::variant<std::vector<VkDescriptorImageInfo>, std::vector<VkDescriptorBufferInfo>, std::vector<VkBufferView>>> descriptorInfos;
  for (auto& [binding, info] : *bindings) {
    switch (info.nameHash) {
      case Tools::hash("albedo"): {
        std::variant<std::vector<VkDescriptorImageInfo>, std::vector<VkDescriptorBufferInfo>, std::vector<VkBufferView>>& data = descriptorInfos[binding];
        if (!std::holds_alternative<std::vector<VkDescriptorImageInfo>>(data))
          data.emplace<std::vector<VkDescriptorImageInfo>>();
        std::get<std::vector<VkDescriptorImageInfo>>(data).push_back({
          .sampler     = material->getAlbedoTexture()->getSampler(),
          .imageView   = material->getAlbedoTexture()->getImageView(),
          .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        });
        break;
      }
      case RenderGraph::getImageId(RenderGraph::ShadowDepth): {
        std::variant<std::vector<VkDescriptorImageInfo>, std::vector<VkDescriptorBufferInfo>, std::vector<VkBufferView>>& data = descriptorInfos[binding];
        if (!std::holds_alternative<std::vector<VkDescriptorImageInfo>>(data))
          data.emplace<std::vector<VkDescriptorImageInfo>>();
        std::get<std::vector<VkDescriptorImageInfo>>(data).push_back({
          .sampler     = *graph.device->getSampler(),
          .imageView   = graph.getImage(RenderGraph::getImageId(RenderGraph::ShadowDepth)).image->getImageView(),
          .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        });
        break;
      }
      case Tools::hash("lightData"): {
        const glm::mat4x4 projectionMatrix = glm::orthoRH_ZO(-1.f, 1.f, -1.f, 1.f, -15.f, 15.f);
        const glm::mat4x4 viewMatrix       = glm::lookAtRH(glm::vec3(1, 10, 1), glm::vec3(0, .25, 0), glm::vec3(0, 0, -1));
        const MaterialData materialData {projectionMatrix * viewMatrix};
        uniformBuffer->update(materialData);
        std::variant<std::vector<VkDescriptorImageInfo>, std::vector<VkDescriptorBufferInfo>, std::vector<VkBufferView>>& data = descriptorInfos[binding];
        if (!std::holds_alternative<std::vector<VkDescriptorBufferInfo>>(data))
          data.emplace<std::vector<VkDescriptorBufferInfo>>();
        std::get<std::vector<VkDescriptorBufferInfo>>(data).push_back({
          .buffer = uniformBuffer->getBuffer(),
          .offset = 0,
          .range  = uniformBuffer->getSize()
        });
        break;
      }
      default:
#if BOOTANICAL_GARDENS_ENABLE_READABLE_SHADER_VARIABLE_NAMES
        GraphicsInstance::showError("unknown object " + info.name);
#else
        GraphicsInstance::showError("unknown object with hash " + std::to_string(info.nameHash));
#endif
        break;
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
      std::vector<VkDescriptorImageInfo>& data = std::get<std::vector<VkDescriptorImageInfo>>(info);
      auto* pInfo = static_cast<VkDescriptorImageInfo*>(miscMemoryPool.emplace_back(malloc(data.size() * sizeof(VkDescriptorImageInfo))));
      memcpy(pInfo, data.data(), data.size() * sizeof(VkDescriptorImageInfo));
      write.pImageInfo = pInfo;
    }
    else if (std::holds_alternative<std::vector<VkDescriptorBufferInfo>>(info)) {
      std::vector<VkDescriptorBufferInfo>& data = std::get<std::vector<VkDescriptorBufferInfo>>(info);
      auto* pInfo = static_cast<VkDescriptorBufferInfo*>(miscMemoryPool.emplace_back(malloc(data.size() * sizeof(VkDescriptorBufferInfo))));
      memcpy(pInfo, data.data(), data.size() * sizeof(VkDescriptorBufferInfo));
      write.pBufferInfo = pInfo;
    }
    else if (std::holds_alternative<std::vector<VkBufferView>>(info)) {
      std::vector<VkBufferView>& data = std::get<std::vector<VkBufferView>>(info);
      auto* pInfo = static_cast<VkBufferView*>(miscMemoryPool.emplace_back(malloc(data.size() * sizeof(VkBufferView))));
      memcpy(pInfo, data.data(), data.size() * sizeof(VkBufferView));
      write.pTexelBufferView = pInfo;
    }

    // Generate a VkWriteDescriptorSet for each descriptor set.
    for (uint64_t i{}; i < descriptorSets.size(); ++i) {
      write.dstSet = *getDescriptorSet(i);
      writes[offset++] = write;
    }
  }
}

void Pipeline::update() {}

Pipeline::~Pipeline() {
  vkDestroyPipelineLayout(device->device, layout, nullptr);
  layout = VK_NULL_HANDLE;
  vkDestroyPipeline(device->device, pipeline, nullptr);
  pipeline = VK_NULL_HANDLE;
}

VkPipeline Pipeline::getPipeline() const { return pipeline; }
VkPipelineLayout Pipeline::getLayout() const { return layout; }
std::shared_ptr<Material> Pipeline::getMaterial() const { return material; }