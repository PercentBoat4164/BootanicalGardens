#include "RenderPass.hpp"

#include "src/RenderEngine/GraphicsDevice.hpp"

#include <volk/volk.h>

uint64_t RenderPass::computeRenderPassCompatibility(const VkRenderPassCreateInfo& createInfo) {
  uint32_t index{};
  uint64_t compatibility{};
  for (uint32_t subpassIndex{}; subpassIndex < createInfo.subpassCount; ++subpassIndex) {
    const VkSubpassDescription& subpass = createInfo.pSubpasses[subpassIndex];
    for (uint32_t attachmentReferenceIndex{}; attachmentReferenceIndex < subpass.colorAttachmentCount; ++attachmentReferenceIndex) {
      if (const uint32_t attachmentIndex = subpass.pColorAttachments[attachmentReferenceIndex].attachment; attachmentIndex != VK_ATTACHMENT_UNUSED) {
        const VkAttachmentDescription& attachment = createInfo.pAttachments[attachmentIndex];
        compatibility ^= rollingShiftLeft(static_cast<uint32_t>(attachment.format) * static_cast<uint32_t>(attachment.samples), ++index);
      }
      if (subpass.pResolveAttachments == nullptr) continue;
      const uint32_t attachmentIndex = subpass.pResolveAttachments[attachmentReferenceIndex].attachment;
      if (attachmentIndex == VK_ATTACHMENT_UNUSED) continue;
      const VkAttachmentDescription& attachment = createInfo.pAttachments[attachmentIndex];
      compatibility ^= rollingShiftLeft(static_cast<uint32_t>(attachment.format) * static_cast<uint32_t>(attachment.samples), ++index);
    }
    for (uint32_t attachmentReferenceIndex{}; attachmentReferenceIndex < subpass.inputAttachmentCount; ++attachmentReferenceIndex) {
      const uint32_t attachmentIndex = subpass.pInputAttachments[attachmentReferenceIndex].attachment;
      if (attachmentIndex == VK_ATTACHMENT_UNUSED) continue;
      const VkAttachmentDescription& attachment = createInfo.pAttachments[attachmentIndex];
      compatibility ^= rollingShiftLeft(static_cast<uint32_t>(attachment.format) * static_cast<uint32_t>(attachment.samples), ++index);
    }
    if (subpass.pDepthStencilAttachment != nullptr) {
      const uint32_t attachmentIndex = subpass.pDepthStencilAttachment->attachment;
      if (attachmentIndex == VK_ATTACHMENT_UNUSED) continue;
      const VkAttachmentDescription& attachment = createInfo.pAttachments[attachmentIndex];
      compatibility ^= static_cast<uint32_t>(attachment.format) * static_cast<uint32_t>(attachment.samples);
    }
  }
  return compatibility;
}

RenderPass::RenderPass(RenderGraph& graph, const MeshFilter meshFilter) : graph(graph), meshFilter(meshFilter) {}

RenderPass::~RenderPass() {
  if (renderPass != VK_NULL_HANDLE) {
    vkDestroyRenderPass(graph.device->device, renderPass, nullptr);
    renderPass = VK_NULL_HANDLE;
  }
}

void RenderPass::addMesh(const std::shared_ptr<Mesh>& mesh) {
  meshes.insert(mesh);
}

void RenderPass::removeMesh(const std::shared_ptr<Mesh>& mesh) {
  meshes.erase(mesh);
}

VkRenderPass RenderPass::getRenderPass() const { return renderPass; }
std::shared_ptr<Framebuffer> RenderPass::getFramebuffer() const { return framebuffer; }