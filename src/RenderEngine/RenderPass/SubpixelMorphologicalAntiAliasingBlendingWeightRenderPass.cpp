#include "SubpixelMorphologicalAntiAliasingBlendingWeightRenderPass.hpp"

#include "src/RenderEngine/CommandBuffer.hpp"
#include "src/RenderEngine/Pipeline/Pipeline.hpp"
#include "src/RenderEngine/Resources/UniformBuffer.hpp"

#include <volk/volk.h>

SubpixelMorphologicalAntiAliasingBlendingWeightRenderPass::SubpixelMorphologicalAntiAliasingBlendingWeightRenderPass(RenderGraph& graph) : RenderPass(graph) {
  material = graph.device->getJSONNonObjectMaterial(6);  // Subpixel Morphological Anti-Aliasing | Blending Weight

  const RenderGraph::ImageParameters parameters {
#if !defined(NDEBUG)
    .name = "SMAABlend",
#endif
    .layers = 1,
    .mipLevels = 1,
    .usage = 0,
    .format = VK_FORMAT_R8G8B8A8_UNORM,
    .sampleCount = VK_SAMPLE_COUNT_1_BIT,
    .resolution = graph.settings.renderResolution
  };
  graph.setImageParameters(RenderGraph::getAttachmentId("SMAABlend"), parameters);
}

void SubpixelMorphologicalAntiAliasingBlendingWeightRenderPass::setup() {
  pipelines.clear();
  materialRemap.clear();

  pipelines.emplace(material, nullptr);

  RenderPass::setup(pipelines | std::ranges::views::keys);
}

void SubpixelMorphologicalAntiAliasingBlendingWeightRenderPass::bake(const std::vector<VkAttachmentDescription>& attachmentDescriptions, const std::vector<const Image*>& images) {
  std::vector<VkAttachmentReference> attachmentReferences(attachmentDescriptions.size());
  uint32_t i{~0U};
  for (VkAttachmentReference& attachmentReference: attachmentReferences) {
    attachmentReference.attachment = ++i;
    attachmentReference.layout     = attachmentDescriptions[i].initialLayout;
  }

  const std::array subpassDescriptions{  /**@todo: Once multiple subpasses can be properly handled, this can be fully automated by a function defined in the RenderPass class.*/
    VkSubpassDescription{
      .flags                   = 0,
      .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .inputAttachmentCount    = inputAttachmentCount,
      .pInputAttachments       = inputAttachmentOffset == ~0U ? nullptr : &attachmentReferences[inputAttachmentOffset],
      .colorAttachmentCount    = colorAttachmentCount,
      .pColorAttachments       = colorAttachmentOffset == ~0U ? nullptr : &attachmentReferences[colorAttachmentOffset],
      .pResolveAttachments     = nullptr,
      .pDepthStencilAttachment = depthStencilAttachmentOffset == ~0U ? nullptr : &attachmentReferences[depthStencilAttachmentOffset],
      .preserveAttachmentCount = 0,
      .pPreserveAttachments    = nullptr
    }
  };

  const VkRenderPassCreateInfo renderPassCreateInfo{
    .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
    .pNext           = nullptr,
    .flags           = 0,
    .attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size()),
    .pAttachments    = attachmentDescriptions.data(),
    .subpassCount    = static_cast<uint32_t>(subpassDescriptions.size()),
    .pSubpasses      = subpassDescriptions.data(),
    .dependencyCount = 0,
    .pDependencies   = nullptr
  };
  setRenderPassInfo(renderPassCreateInfo, images);
  if (const VkResult result = vkCreateRenderPass(graph.device->device, &renderPassCreateInfo, nullptr, &renderPass); result != VK_SUCCESS) GraphicsInstance::showError(result, "failed to create render pass");
#if VK_EXT_debug_utils & BOOTANICAL_GARDENS_ENABLE_VULKAN_DEBUG_UTILS
  if (GraphicsInstance::extensionEnabled(Tools::hash(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))) {
    const VkDebugUtilsObjectNameInfoEXT nameInfo {
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
      .pNext = nullptr,
      .objectType = VK_OBJECT_TYPE_RENDER_PASS,
      .objectHandle = std::bit_cast<uint64_t>(renderPass),
      .pObjectName = name.c_str()
    };
    if (const VkResult result = vkSetDebugUtilsObjectNameEXT(graph.device->device, &nameInfo); result != VK_SUCCESS) GraphicsInstance::showError(result, "failed to set debug utils object name");
  }
#endif

  for (auto& [material, pipeline]: pipelines) pipeline = graph.device->getPipeline(material, compatibility);
  framebuffer = std::make_unique<Framebuffer>(graph.device, images, renderPass);
  uniformBuffer = std::make_unique<UniformBuffer<PassData>>(graph.device, "SMAA Blending Weight Render Pass | Uniform Buffer");
}

void SubpixelMorphologicalAntiAliasingBlendingWeightRenderPass::writeDescriptorSets(std::deque<std::tuple<void*, std::function<void(void*)>>>& miscMemoryPool, std::vector<VkWriteDescriptorSet>& writes) {
  const uint32_t offset = writes.size();
  writes.resize(offset + descriptorSets.size(), {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pNext = nullptr,
      .dstBinding = 0,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .pImageInfo = nullptr,
      .pBufferInfo = static_cast<VkDescriptorBufferInfo*>(std::get<0>(miscMemoryPool.emplace_back(new VkDescriptorBufferInfo{
        .buffer = uniformBuffer->getBuffer(),
        .offset = 0,
        .range = uniformBuffer->getSize()
      }, [](void* mem) { delete static_cast<VkDescriptorBufferInfo*>(mem); }))),
      .pTexelBufferView = nullptr
  });
  for (uint64_t i{}; i < descriptorSets.size(); ++i) writes[offset + i].dstSet = getDescriptorSet(i);
}

void SubpixelMorphologicalAntiAliasingBlendingWeightRenderPass::update() {
  static std::uint64_t oldWidth = 0;
  static std::uint64_t oldHeight = 0;
  if (oldWidth == graph.settings.renderResolution.width && oldHeight == graph.settings.renderResolution.height) return;
  const PassData passData {
    .resolution = glm::vec4{
      1.0 / graph.settings.renderResolution.width,
      1.0 / graph.settings.renderResolution.height,
      graph.settings.renderResolution.width,
      graph.settings.renderResolution.height
    },
    .viewProj = {},
    .prevViewProj = {},
    .guiOrtho = {},
    .parameters = {
      .threshold = 0.1,
      .depthThreshold = 0.01,
      .maxSearchSteps = 32,
      .maxSearchStepsDiag = 16,
      .cornerRounding = 25,
      .pad0 = 0,
      .pad1 = 0,
      .pad2 = 0
    },
    .subsampleIndices = {0, 0, 0, 0},
    .predicationThreshold = 0.0,
    .predicationScale = 0.0,
    .predicationStrength = 0.0,
    .reprojWeigthScale = 0.0
  };
  uniformBuffer->update(passData);
  oldWidth = graph.settings.renderResolution.width;
  oldHeight = graph.settings.renderResolution.height;
}

void SubpixelMorphologicalAntiAliasingBlendingWeightRenderPass::execute(CommandBuffer& commandBuffer) {
  const std::uint64_t frameIndex = graph.getFrameIndex();
  const Pipeline* pipeline = pipelines.at(material);
  commandBuffer.record<CommandBuffer::BeginRenderPass>(this, clearValues);
  commandBuffer.record<CommandBuffer::BindPipeline>(pipeline);
  commandBuffer.record<CommandBuffer::BindDescriptorSets>(std::array{getDescriptorSet(frameIndex), pipeline->getDescriptorSet(frameIndex)}, 1);
  commandBuffer.record<CommandBuffer::Draw>(3);
  commandBuffer.record<CommandBuffer::EndRenderPass>();
}
