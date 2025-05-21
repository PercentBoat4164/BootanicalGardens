#include "Pipeline.hpp"

#include "GraphicsInstance.hpp"
#include "RenderPass/RenderPass.hpp"
#include "Renderable/Material.hpp"
#include "Renderable/Vertex.hpp"
#include "Shader.hpp"

#include <magic_enum/magic_enum.hpp>
#include <volk/volk.h>

#include <ranges>

Pipeline::Pipeline(const std::shared_ptr<GraphicsDevice>& device, std::shared_ptr<const Material> material, std::shared_ptr<const RenderPass> renderPass, VkPipelineLayout layout) : device(device), layout(layout) {
  shaders = material->getShaders();
  if (shaders.empty()) GraphicsInstance::showError("A pipeline must be created with at least one shader.");
  bindPoint = shaders.back()->getBindPoint();
  for (const std::shared_ptr<Shader>& shader : shaders)
    if (shader->getBindPoint() != bindPoint) GraphicsInstance::showError("Attempted to create a pipeline of inferred bind point " + std::string(magic_enum::enum_name(bindPoint)) + " using a shader with bind point " + std::string(magic_enum::enum_name(shader->getBindPoint())) + ".");

  if (bindPoint == VK_PIPELINE_BIND_POINT_COMPUTE) {
    const VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo {
      .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .pNext               = nullptr,
      .flags               = 0,
      .stage               = shaders.back()->getStage(),
      .module              = shaders.back()->getModule(),
      .pName               = shaders.back()->getEntryPoint().data(),
      .pSpecializationInfo = nullptr
    };
    const VkComputePipelineCreateInfo pipelineCreateInfo {
      .sType              = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
      .pNext              = nullptr,
      .flags              = 0,
      .stage              = pipelineShaderStageCreateInfo,
      .layout             = layout,
      .basePipelineHandle = VK_NULL_HANDLE,
      .basePipelineIndex  = 0
    };
    GraphicsInstance::showError(vkCreateComputePipelines(device->device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline), "Failed to create compute pipeline.");
  } else {
    std::vector<VkPipelineShaderStageCreateInfo> stages{shaders.size()};
    for (std::tuple zip : std::ranges::views::zip(shaders, stages)) {
      std::get<1>(zip) = {
        .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext               = nullptr,
        .flags               = 0,
        .stage               = std::get<0>(zip)->getStage(),
        .module              = std::get<0>(zip)->getModule(),
        .pName               = std::get<0>(zip)->getEntryPoint().data(),
        .pSpecializationInfo = VK_NULL_HANDLE
      };
    }
    constexpr VkVertexInputBindingDescription bindingDescription = Vertex::getBindingDescription();
    const std::vector<VkVertexInputAttributeDescription> attributeDescriptions = Vertex::getAttributeDescriptions();
    const VkPipelineVertexInputStateCreateInfo vertexInputState{
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
      .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,  /**@todo: Support multiple topologies. Low priority. Topology is recorded on a per-mesh basis.*/
      .primitiveRestartEnable = VK_FALSE
    };
    constexpr VkViewport viewport {
      .x        = 0,
      .y        = 0,
      .width    = 1,
      .height   = 1,
      .minDepth = 0,
      .maxDepth = 1
    };
    constexpr VkRect2D scissor {
      .offset = {
        .x = 0,
        .y = 0
      },
      .extent = {
        .width  = 1,
        .height = 1
      } 
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
          .failOp      = VK_STENCIL_OP_KEEP,
          .passOp      = VK_STENCIL_OP_KEEP,
          .depthFailOp = VK_STENCIL_OP_KEEP,
          .compareOp   = VK_COMPARE_OP_LESS,
          .compareMask = 0,
          .writeMask   = 0,
          .reference   = 0
        },
        .minDepthBounds = 0.0,
        .maxDepthBounds = 1.0
    };
    const std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates(1, {  /**@todo: Make this reflect the number of color attachments the renderPass has.*/
      .blendEnable = VK_FALSE,
      .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
      .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
      .colorBlendOp = VK_BLEND_OP_ADD,
      .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
      .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
      .alphaBlendOp = VK_BLEND_OP_ADD,
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    });
    const VkPipelineColorBlendStateCreateInfo colorBlendState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .logicOpEnable = VK_FALSE,
      .logicOp = VK_LOGIC_OP_COPY,
      .attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size()),
      .pAttachments = blendAttachmentStates.data(),
      .blendConstants = {0, 0, 0, 0}
    };
    constexpr VkDynamicState dynamicStates[]{
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    const VkPipelineDynamicStateCreateInfo dynamicState{
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext             = nullptr,
        .flags             = 0,
        .dynamicStateCount = std::extent_v<decltype(dynamicStates)>,
        .pDynamicStates    = dynamicStates
    };
    const VkGraphicsPipelineCreateInfo pipelineCreateInfo{
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
  }
  descriptorSets = device->perMaterialDescriptorAllocator.allocate(RenderGraph::FRAMES_IN_FLIGHT);
}

Pipeline::~Pipeline() {
  vkDestroyPipeline(device->device, pipeline, nullptr);
  pipeline = VK_NULL_HANDLE;
}

VkPipeline Pipeline::getPipeline() const { return pipeline; }
VkPipelineLayout Pipeline::getLayout() const { return layout; }
std::shared_ptr<VkDescriptorSet> Pipeline::getDescriptorSet(const uint64_t frameIndex) const { return descriptorSets[frameIndex]; }