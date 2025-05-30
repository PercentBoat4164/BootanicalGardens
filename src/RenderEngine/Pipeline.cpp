#include "Pipeline.hpp"

#include "GraphicsInstance.hpp"
#include "RenderPass/RenderPass.hpp"
#include "Renderable/Material.hpp"
#include "Renderable/Texture.hpp"
#include "Renderable/Vertex.hpp"
#include "Shader.hpp"

#include <magic_enum/magic_enum.hpp>
#include <volk/volk.h>

#include <ranges>

Pipeline::Pipeline(const std::shared_ptr<GraphicsDevice>& device, const std::shared_ptr<const Material>& material, const std::shared_ptr<const RenderPass>& renderPass) : device(device), material(material), bindPoint(VK_PIPELINE_BIND_POINT_GRAPHICS) {
  const std::shared_ptr<const Shader> vertexShader   = material->getVertexShader();
  const std::shared_ptr<const Shader> fragmentShader = material->getFragmentShader();
  const std::vector<std::shared_ptr<const Shader>> shaders = {vertexShader, fragmentShader};

  // Obtain reflected descriptor set data
  std::vector<std::vector<SpvReflectDescriptorSet*>> shaderSets(shaders.size());  // Per-shader sets
  for (uint32_t i{}; i < shaders.size(); ++i) {
    uint32_t count;
    const spv_reflect::ShaderModule* reflectedData = shaders[i]->getReflectedData();
    reflectedData->EnumerateDescriptorSets(&count, nullptr);
    shaderSets[i] = std::vector<SpvReflectDescriptorSet*>(count);
    reflectedData->EnumerateDescriptorSets(&count, shaderSets[i].data());
  }

  // Find the greatest set number
  uint32_t maxSet = 0;  // Highest descriptor set index
  for (const std::vector<SpvReflectDescriptorSet*>& sets : shaderSets) for (const SpvReflectDescriptorSet* set : sets) maxSet = std::max(maxSet, set->set + 1);

  // Generate bindings
  std::vector<std::vector<VkDescriptorSetLayoutBinding>> setBindings(maxSet);  // Per-set bindings
  for (uint32_t i{}; i < shaderSets.size(); ++i) {
    for (const SpvReflectDescriptorSet* set : shaderSets[i]) {
      for (uint32_t j{}; j < set->binding_count; ++j) {
        const SpvReflectDescriptorBinding* binding          = set->bindings[j];  // This reflected set's binding
        const uint32_t thisBinding                          = binding->binding;  // This reflected binding's bind location
        std::vector<VkDescriptorSetLayoutBinding>& bindings = setBindings[set->set];  // This set's bindings
        if (auto it = std::ranges::find_if(bindings, [thisBinding](const VkDescriptorSetLayoutBinding& x) { return x.binding == thisBinding; }); it != bindings.end()) {
          // If the binding is already in this set, update it.
          it->descriptorCount = std::max(binding->count, it->descriptorCount);
          it->stageFlags |= shaders[i]->getStage();
        } else {
          // If the binding is not already in this set, add it.
          bindings.push_back({
            .binding            = thisBinding,
            .descriptorType     = static_cast<VkDescriptorType>(binding->descriptor_type),
            .descriptorCount    = binding->count,
            .stageFlags         = static_cast<uint32_t>(shaders[i]->getStage()),
            .pImmutableSamplers = nullptr
          });
        }
      }
    }
  }

  // Create the descriptor layouts
  std::vector<VkDescriptorSetLayout> layouts(maxSet);
  for (uint32_t i{}; i < setBindings.size(); ++i) {
    /**@todo: Build a system similar to what exists for VkSampler objects to prevent duplication of VkDescriptorSetLayout objects.*/
    VkDescriptorSetLayoutCreateInfo createInfo {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .bindingCount = static_cast<uint32_t>(setBindings[i].size()),
      .pBindings = setBindings[i].data()
    };
    if (const VkResult result = vkCreateDescriptorSetLayout(device->device, &createInfo, nullptr, &layouts[i]); result != VK_SUCCESS) GraphicsInstance::showError(result, "failed to create descriptor set layout");
  }

  // Create the pipeline layout
  VkPipelineLayoutCreateInfo createInfo {
      .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .pNext                  = nullptr,
      .flags                  = 0,
      .setLayoutCount         = static_cast<uint32_t>(layouts.size()),
      .pSetLayouts            = layouts.data(),
      .pushConstantRangeCount = 0,
      .pPushConstantRanges    = nullptr
  };
  if (const VkResult result = vkCreatePipelineLayout(device->device, &createInfo, nullptr, &layout); result != VK_SUCCESS) GraphicsInstance::showError(result, "failed to create pipeline layout");

  // Destroy the descriptor layout
  for (const VkDescriptorSetLayout& layout: layouts) vkDestroyDescriptorSetLayout(device->device, layout, nullptr);

  std::vector<VkPipelineShaderStageCreateInfo> stages(shaders.size());
  for (uint32_t i{}; i < shaders.size(); ++i) {
    const auto& shader = shaders[i];
    stages[i] = VkPipelineShaderStageCreateInfo {
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
  constexpr VkVertexInputBindingDescription bindingDescription               = Vertex::getBindingDescription();
  const std::vector<VkVertexInputAttributeDescription> attributeDescriptions = Vertex::getAttributeDescriptions();
  const VkPipelineVertexInputStateCreateInfo vertexInputState {
      .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .pNext                           = nullptr,
      .flags                           = 0,
      .vertexBindingDescriptionCount   = 1,
      .pVertexBindingDescriptions      = &bindingDescription,
      .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
      .pVertexAttributeDescriptions    = attributeDescriptions.data()
  };
  constexpr VkPipelineInputAssemblyStateCreateInfo inputAssemblyState {
      .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .pNext                  = nullptr,
      .flags                  = 0,
      .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, /**@todo: Support multiple topologies. Low priority. Topology is recorded on a per-mesh basis.*/
      .primitiveRestartEnable = VK_FALSE
  };
  constexpr VkViewport viewport{
      .x        = 0,
      .y        = 0,
      .width    = 1,
      .height   = 1,
      .minDepth = 0,
      .maxDepth = 1
  };
  constexpr VkRect2D scissor{
      .offset = {
          .x = 0,
          .y = 0
      },
      .extent = {.width = 1, .height = 1}
  };
  const VkPipelineViewportStateCreateInfo viewPortState {
      .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .pNext         = nullptr,
      .flags         = 0,
      .viewportCount = 1,
      .pViewports    = &viewport,
      .scissorCount  = 1,
      .pScissors     = &scissor
  };
  const VkPipelineRasterizationStateCreateInfo rasterizationState{
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
  };
  constexpr VkPipelineMultisampleStateCreateInfo multisampleState{
      .sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .pNext                 = nullptr,
      .flags                 = 0,
      .rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
      .sampleShadingEnable   = VK_FALSE,
      .minSampleShading      = 1.0,
      .pSampleMask           = nullptr,
      .alphaToCoverageEnable = VK_FALSE,
      .alphaToOneEnable      = VK_FALSE
  };
  constexpr VkPipelineDepthStencilStateCreateInfo depthStencilState{
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
      .back = {
        .failOp = VK_STENCIL_OP_KEEP,
        .passOp = VK_STENCIL_OP_KEEP,
        .depthFailOp = VK_STENCIL_OP_KEEP,
        .compareOp = VK_COMPARE_OP_LESS,
        .compareMask = 0,
        .writeMask = 0,
        .reference = 0
      },
      .minDepthBounds = 0.0,
      .maxDepthBounds = 1.0
  };
  const std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates(1, {/**@todo: Make this reflect the number of color attachments the renderPass has.*/
      .blendEnable         = VK_FALSE,
      .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
      .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
      .colorBlendOp        = VK_BLEND_OP_ADD,
      .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
      .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
      .alphaBlendOp        = VK_BLEND_OP_ADD,
      .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
  });
  const VkPipelineColorBlendStateCreateInfo colorBlendState {
      .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .pNext           = nullptr,
      .flags           = 0,
      .logicOpEnable   = VK_FALSE,
      .logicOp         = VK_LOGIC_OP_COPY,
      .attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size()),
      .pAttachments    = blendAttachmentStates.data(),
      .blendConstants  = {0, 0, 0, 0}
  };
  constexpr VkDynamicState dynamicStates[] {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR
  };
  const VkPipelineDynamicStateCreateInfo dynamicState {
      .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .pNext             = nullptr,
      .flags             = 0,
      .dynamicStateCount = std::extent_v<decltype(dynamicStates)>,
      .pDynamicStates    = dynamicStates
  };
  const VkGraphicsPipelineCreateInfo pipelineCreateInfo {
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
      .subpass             = 0,
      .basePipelineHandle  = VK_NULL_HANDLE,
      .basePipelineIndex   = -1,
  };
  if (const VkResult result = vkCreateGraphicsPipelines(device->device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline); result != VK_SUCCESS) GraphicsInstance::showError(result, "Failed to create graphics pipeline.");

  descriptorSets = device->perMaterialDescriptorAllocator.allocate(RenderGraph::FRAMES_IN_FLIGHT);
  VkDescriptorImageInfo imageInfo{
      .sampler     = material->getAlbedoTexture()->getSampler(),
      .imageView   = material->getAlbedoTexture()->getImageView(),
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
  };
  std::vector<VkWriteDescriptorSet> writeDescriptorSet(descriptorSets.size(), {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .pNext = nullptr, .dstSet = VK_NULL_HANDLE, .dstBinding = 0, .dstArrayElement = 0, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .pImageInfo = &imageInfo, .pBufferInfo = nullptr, .pTexelBufferView = nullptr});
  for (uint64_t i{}; i < descriptorSets.size(); ++i)
    writeDescriptorSet[i].dstSet = *descriptorSets[i];
  vkUpdateDescriptorSets(device->device, writeDescriptorSet.size(), writeDescriptorSet.data(), 0, nullptr);
}

Pipeline::~Pipeline() {
  vkDestroyPipelineLayout(device->device, layout, nullptr);
  layout = VK_NULL_HANDLE;
  vkDestroyPipeline(device->device, pipeline, nullptr);
  pipeline = VK_NULL_HANDLE;
}

VkPipeline Pipeline::getPipeline() const { return pipeline; }
VkPipelineLayout Pipeline::getLayout() const { return layout; }
std::shared_ptr<VkDescriptorSet> Pipeline::getDescriptorSet(const uint64_t frameIndex) const { return descriptorSets[frameIndex]; }
void Pipeline::update() {}