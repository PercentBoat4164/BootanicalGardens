#include "CollectShadowsRenderPass.hpp"

#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "src/RenderEngine/GraphicsDevice.hpp"
#include "src/RenderEngine/GraphicsInstance.hpp"
#include "src/RenderEngine/Pipeline.hpp"
#include "src/RenderEngine/Renderable/Material.hpp"
#include "src/RenderEngine/Resources/Image.hpp"
#include "src/RenderEngine/Resources/UniformBuffer.hpp"

#include <filesystem>
#include <volk/volk.h>

CollectShadowsRenderPass::CollectShadowsRenderPass(RenderGraph& graph) : RenderPass(graph, OpaqueBit) {
  material = std::make_shared<Material>(
    std::make_shared<Shader>(graph.device, std::filesystem::canonical("../res/shaders/deferred.vert")),
    std::make_shared<Shader>(graph.device, std::filesystem::canonical("../res/shaders/deferred.frag"))
  );
}

std::vector<std::pair<RenderGraph::ImageID, RenderGraph::ImageAccess>> CollectShadowsRenderPass::declareAccesses() {
  std::array materials{material};
  setup(materials);  /*@todo: Perform setup once during RenderGraph baking time. This function should just return the imageAccesses.*/
  return imageAccesses;
}

void CollectShadowsRenderPass::bake(const std::vector<VkAttachmentDescription>& attachmentDescriptions, const std::vector<std::shared_ptr<Image>>& images) {
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
      .objectHandle = reinterpret_cast<uint64_t>(renderPass),
      .pObjectName = "Collect Shadows Render Pass"
    };
    if (const VkResult result = vkSetDebugUtilsObjectNameEXT(graph.device->device, &nameInfo); result != VK_SUCCESS) GraphicsInstance::showError(result, "failed to set debug utils object name");
  }
#endif

  framebuffer = std::make_shared<Framebuffer>(graph.device, images, renderPass);
  pipelines[material] = std::make_shared<Pipeline>(graph.device, material);

  uniformBuffer = std::make_shared<UniformBuffer<PassData>>(graph.device, "Collect Shadows Pass | Uniform Buffer");
}

void CollectShadowsRenderPass::writeDescriptorSets(std::vector<void*>& miscMemoryPool, std::vector<VkWriteDescriptorSet>& writes, const RenderGraph& graph) {
  // The g-buffer albedo
  uint32_t offset = writes.size();
  writes.resize(offset + descriptorSets.size(), {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pNext = nullptr,
      .dstBinding = 0,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
      .pImageInfo = static_cast<VkDescriptorImageInfo*>(miscMemoryPool.emplace_back(new VkDescriptorImageInfo{
        .sampler     = *graph.device->getSampler(),
        .imageView   = graph.getImage(RenderGraph::getImageId(RenderGraph::GBufferAlbedo)).image->getImageView(),
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
      })),
      .pBufferInfo = nullptr,
      .pTexelBufferView = nullptr
  });
  for (uint64_t i{}; i < descriptorSets.size(); ++i) writes[offset + i].dstSet = *getDescriptorSet(i);
  // The g-buffer position
  offset = writes.size();
  writes.resize(offset + descriptorSets.size(), {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pNext = nullptr,
      .dstBinding = 1,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
      .pImageInfo = static_cast<VkDescriptorImageInfo*>(miscMemoryPool.emplace_back(new VkDescriptorImageInfo{
        .sampler     = *graph.device->getSampler(),
        .imageView   = graph.getImage(RenderGraph::getImageId(RenderGraph::GBufferPosition)).image->getImageView(),
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
      })),
      .pBufferInfo = nullptr,
      .pTexelBufferView = nullptr
  });
  for (uint64_t i{}; i < descriptorSets.size(); ++i) writes[offset + i].dstSet = *getDescriptorSet(i);
  // The g-buffer normal
  offset = writes.size();
  writes.resize(offset + descriptorSets.size(), {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pNext = nullptr,
      .dstBinding = 2,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
      .pImageInfo = static_cast<VkDescriptorImageInfo*>(miscMemoryPool.emplace_back(new VkDescriptorImageInfo{
        .sampler     = *graph.device->getSampler(),
        .imageView   = graph.getImage(RenderGraph::getImageId(RenderGraph::GBufferNormal)).image->getImageView(),
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
      })),
      .pBufferInfo = nullptr,
      .pTexelBufferView = nullptr
  });
  for (uint64_t i{}; i < descriptorSets.size(); ++i) writes[offset + i].dstSet = *getDescriptorSet(i);
}

std::optional<std::pair<RenderGraph::ImageID, RenderGraph::ImageAccess>> CollectShadowsRenderPass::getDepthStencilAttachmentAccess() {
  return{};
}

void CollectShadowsRenderPass::update() {
  const PassData passData {
    .view_ViewMatrixInverse       = glm::inverse(glm::lookAtRH(glm::vec3(1, 1, 1), glm::vec3(0, .25, 0), glm::vec3(0, -1, 0))),
    .view_ProjectionMatrixInverse = glm::inverse(glm::perspectiveRH_ZO(glm::radians(60.0f), 8.0f / 6.0f, 1.0f, 2.0f))
  };
  uniformBuffer->update(passData);
}

void CollectShadowsRenderPass::execute(CommandBuffer& commandBuffer) {
  commandBuffer.record<CommandBuffer::BeginRenderPass>(shared_from_this());
  std::shared_ptr<Pipeline> pipeline = pipelines.at(material);
  const uint64_t frameIndex = graph.getFrameIndex();
  commandBuffer.record<CommandBuffer::BindPipeline>(pipeline);
  commandBuffer.record<CommandBuffer::BindDescriptorSets>(std::vector{*getDescriptorSet(frameIndex), *pipeline->getDescriptorSet(frameIndex)}, 1);
  commandBuffer.record<CommandBuffer::Draw>(3);  // No vertex buffer needs to be bound for this call because the vertex shader generates the required vertex attributes automatically.
  commandBuffer.record<CommandBuffer::EndRenderPass>();
}