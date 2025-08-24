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

std::vector<std::pair<RenderGraph::AttachmentID, RenderGraph::AttachmentDeclaration>> CollectShadowsRenderPass::declareAttachments() {
  return {
      {RenderGraph::GBufferAlbedo,
          RenderGraph::AttachmentDeclaration{
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .access = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
            .stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE
          }
      }
  };
}

void CollectShadowsRenderPass::bake(const std::vector<VkAttachmentDescription>& attachmentDescriptions, const std::vector<std::shared_ptr<Image>>& images) {
  const std::array attachmentReferences{
    VkAttachmentReference{
      .attachment = 0,
      .layout     = attachmentDescriptions[0].initialLayout,
    }
  };

  const std::array subpassDescriptions{
    VkSubpassDescription{
      .flags                   = 0,
      .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .inputAttachmentCount    = 0,
      .pInputAttachments       = nullptr,
      .colorAttachmentCount    = 1,
      .pColorAttachments       = &attachmentReferences[0],
      .pResolveAttachments     = nullptr,
      .pDepthStencilAttachment = nullptr
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
  // The g-buffer position
  const auto imageInfo = static_cast<VkDescriptorImageInfo*>(miscMemoryPool.emplace_back(new VkDescriptorImageInfo{
    .sampler     = *graph.device->getSampler(),
    .imageView   = graph.getAttachmentImage(RenderGraph::GBufferPosition)->getImageView(),
    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
  }));
  const uint32_t offset = writes.size();
  writes.resize(offset + descriptorSets.size(), {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pNext = nullptr,
      .dstBinding = 0,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .pImageInfo = imageInfo,
      .pBufferInfo = nullptr,
      .pTexelBufferView = nullptr
  });
  for (uint64_t i{}; i < descriptorSets.size(); ++i) writes[offset + i].dstSet = *getDescriptorSet(i);
}

void CollectShadowsRenderPass::update() {
  const PassData passData {
    .view_ViewMatrixInverse       = glm::inverse(glm::lookAtRH(glm::vec3(1, 1, 1), glm::vec3(0, .25, 0), glm::vec3(0, -1, 0))),
    .view_ProjectionMatrixInverse = glm::inverse(glm::perspectiveRH_ZO(glm::radians(60.0f), 8.0f / 6.0f, 1.0f, 2.0f))
  };
  uniformBuffer->update(passData);
}

void CollectShadowsRenderPass::execute(CommandBuffer& commandBuffer) {
  /**@todo: Do not require the hardcoding of this PipelineBarrier.*/
  auto shadowDepth = graph.getAttachmentImage(RenderGraph::getAttachmentId("ShadowDepth"));
  auto renderDepth = graph.getAttachmentImage(RenderGraph::GBufferDepth);
  const std::vector<VkImageMemoryBarrier> imageBarriers {
    {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .pNext = nullptr,
      .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
      .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
      .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      .srcQueueFamilyIndex = graph.device->globalQueueFamilyIndex,
      .dstQueueFamilyIndex = graph.device->globalQueueFamilyIndex,
      .image = shadowDepth->getImage(),
      .subresourceRange = shadowDepth->getWholeRange()
    }, {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .pNext = nullptr,
      .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
      .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
      .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      .srcQueueFamilyIndex = graph.device->globalQueueFamilyIndex,
      .dstQueueFamilyIndex = graph.device->globalQueueFamilyIndex,
      .image = renderDepth->getImage(),
      .subresourceRange = renderDepth->getWholeRange()
    }
  };
  commandBuffer.record<CommandBuffer::PipelineBarrier>(VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, std::vector<VkMemoryBarrier>{}, std::vector<VkBufferMemoryBarrier>{}, imageBarriers);
  commandBuffer.record<CommandBuffer::BeginRenderPass>(shared_from_this());
  std::shared_ptr<Pipeline> pipeline = pipelines[material];
  const uint64_t frameIndex = graph.getFrameIndex();
  commandBuffer.record<CommandBuffer::BindPipeline>(pipeline);
  commandBuffer.record<CommandBuffer::BindDescriptorSets>(std::vector{*getDescriptorSet(frameIndex), *pipeline->getDescriptorSet(frameIndex)}, 1);
  commandBuffer.record<CommandBuffer::Draw>(3);  // No vertex buffer needs to be bound for this call because the vertex shader generates the required vertex attributes automatically.
  commandBuffer.record<CommandBuffer::EndRenderPass>();
}