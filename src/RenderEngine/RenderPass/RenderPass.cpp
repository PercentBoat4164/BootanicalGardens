#include "RenderPass.hpp"

#include "src/RenderEngine/GraphicsDevice.hpp"
#include "src/RenderEngine/GraphicsInstance.hpp"
#include "src/RenderEngine/Renderable/Material.hpp"
#include "src/RenderEngine/Renderable/Mesh.hpp"

#include <volk/volk.h>

/**@todo: Thread this function.*/
void RenderPass::setRenderPassInfo(const VkRenderPassCreateInfo& createInfo, const std::vector<std::shared_ptr<Image>>& images) {
  // Compute the render pass's compatibility. This is given as the hash of the attributes of the render pass that determine compatibility.
  uint32_t index{};
  compatibility = 0;
  for (uint32_t subpassIndex{}; subpassIndex < createInfo.subpassCount; ++subpassIndex) {
    const VkSubpassDescription& subpass = createInfo.pSubpasses[subpassIndex];
    // Hash the color attachments
    for (uint32_t attachmentReferenceIndex{}; attachmentReferenceIndex < subpass.colorAttachmentCount; ++attachmentReferenceIndex) {
      const uint32_t attachmentIndex = subpass.pColorAttachments[attachmentReferenceIndex].attachment;
      if (attachmentIndex == VK_ATTACHMENT_UNUSED) continue;
      const VkAttachmentDescription& attachment = createInfo.pAttachments[attachmentIndex];
      compatibility ^= rollingShiftLeft(static_cast<uint32_t>(attachment.format) * static_cast<uint32_t>(attachment.samples), ++index);
    }
    // Hash the resolve attachments
    if (subpass.pResolveAttachments != nullptr && createInfo.subpassCount > 1) {
      // Vulkan Spec: "As an additional special case, if two render passes have a single subpass, the resolve attachment reference and depth/stencil resolve mode compatibility requirements are ignored."
      for (uint32_t attachmentReferenceIndex{}; attachmentReferenceIndex < subpass.colorAttachmentCount; ++attachmentReferenceIndex) {
        const uint32_t attachmentIndex = subpass.pResolveAttachments[attachmentReferenceIndex].attachment;
        if (attachmentIndex == VK_ATTACHMENT_UNUSED) continue;
        const VkAttachmentDescription& attachment = createInfo.pAttachments[attachmentIndex];
        compatibility ^= rollingShiftLeft(static_cast<uint32_t>(attachment.format) * static_cast<uint32_t>(attachment.samples), ++index);
      }
    }
    // Hash the input attachments
    for (uint32_t attachmentReferenceIndex{}; attachmentReferenceIndex < subpass.inputAttachmentCount; ++attachmentReferenceIndex) {
      const uint32_t attachmentIndex = subpass.pInputAttachments[attachmentReferenceIndex].attachment;
      if (attachmentIndex == VK_ATTACHMENT_UNUSED) continue;
      const VkAttachmentDescription& attachment = createInfo.pAttachments[attachmentIndex];
      compatibility ^= rollingShiftLeft(static_cast<uint32_t>(attachment.format) * static_cast<uint32_t>(attachment.samples), ++index);
    }
    // Hash the depth/stencil attachment
    if (subpass.pDepthStencilAttachment != nullptr || createInfo.subpassCount > 1) {  // Vulkan Spec: "As an additional special case, if two render passes have a single subpass, the resolve attachment reference and depth/stencil resolve mode compatibility requirements are ignored."
      const uint32_t attachmentIndex = subpass.pDepthStencilAttachment->attachment;
      if (attachmentIndex == VK_ATTACHMENT_UNUSED) continue;
      const VkAttachmentDescription& attachment = createInfo.pAttachments[attachmentIndex];
      compatibility ^= static_cast<uint32_t>(attachment.format) * static_cast<uint32_t>(attachment.samples);
    }
  }

  // Fill in the subpass data
  subpassData.resize(createInfo.subpassCount);
  for (uint32_t subpassIndex{}; subpassIndex < createInfo.subpassCount; ++subpassIndex) {
    auto& [colorImages, resolveImages, inputImages, depthImage] = subpassData.at(subpassIndex);
    const VkSubpassDescription& subpass = createInfo.pSubpasses[subpassIndex];
    // Handle color attachments
    colorImages.reserve(subpass.colorAttachmentCount);
    for (uint32_t attachmentReferenceIndex{}; attachmentReferenceIndex < subpass.colorAttachmentCount; ++attachmentReferenceIndex) {
      if (const uint32_t attachmentIndex = subpass.pColorAttachments[attachmentReferenceIndex].attachment; attachmentIndex != VK_ATTACHMENT_UNUSED) colorImages.push_back(images.at(attachmentIndex));
      else colorImages.push_back(nullptr);
    }
    // Handle resolve attachments
    if (subpass.pResolveAttachments != nullptr) {
      resolveImages.reserve(subpass.colorAttachmentCount);
      for (uint32_t attachmentReferenceIndex{}; attachmentReferenceIndex < subpass.colorAttachmentCount; ++attachmentReferenceIndex) {
        if (const uint32_t attachmentIndex = subpass.pResolveAttachments[attachmentReferenceIndex].attachment; attachmentIndex != VK_ATTACHMENT_UNUSED) resolveImages.push_back(images.at(attachmentIndex));
        else resolveImages.push_back(nullptr);
      }
    }
    // Handle input attachments
    inputImages.reserve(subpass.inputAttachmentCount);
    for (uint32_t attachmentReferenceIndex{}; attachmentReferenceIndex < subpass.inputAttachmentCount; ++attachmentReferenceIndex) {
      if (const uint32_t attachmentIndex = subpass.pInputAttachments[attachmentReferenceIndex].attachment; attachmentIndex != VK_ATTACHMENT_UNUSED) inputImages.push_back(images.at(attachmentIndex));
      else inputImages.push_back(nullptr);
    }
    // Handle the depth/stencil attachment
    if (subpass.pDepthStencilAttachment != nullptr) {
      if (const uint32_t attachmentIndex = subpass.pDepthStencilAttachment->attachment; attachmentIndex != VK_ATTACHMENT_UNUSED) depthImage = images.at(attachmentIndex);
    } else depthImage = nullptr;
  }
}

RenderPass::RenderPass(RenderGraph& graph, const MeshFilter meshFilter) : DescriptorSetRequirer(graph.device), graph(graph), meshFilter(meshFilter) {}

RenderPass::~RenderPass() {
  if (renderPass != VK_NULL_HANDLE) {
    vkDestroyRenderPass(graph.device->device, renderPass, nullptr);
    renderPass = VK_NULL_HANDLE;
  }
}

void RenderPass::setup(const std::span<std::shared_ptr<Material>> materials) {
  imageAccesses.clear();
  // Prepare data for this render pass
  if (const std::optional<std::pair<RenderGraph::ImageID, RenderGraph::ImageAccess>> optionalDepthStencilAttachmentAccess = getDepthStencilAttachmentAccess(); optionalDepthStencilAttachmentAccess.has_value()) {
    depthStencilAttachmentOffset = imageAccesses.size();
    auto [id, access] = optionalDepthStencilAttachmentAccess.value();
    imageAccesses.emplace_back(id, access);
  }
  colorAttachmentOffset = imageAccesses.size();
  for (const std::shared_ptr<Material>& material: materials) {
    const std::vector<std::pair<RenderGraph::ImageID, RenderGraph::ImageAccess>>& colorAttachmentAccesses = material->computeColorAttachmentAccesses();
    for (auto& [id, access]: colorAttachmentAccesses) {
      auto it = std::ranges::find(imageAccesses, id, &decltype(imageAccesses)::value_type::first);
      if (it == imageAccesses.end()) imageAccesses.emplace_back(id, access);
      /**@todo: If the order of rendering is known (e.g. if the meshes are sorted), then this error can go away and we can just use the first and last layouts that the image is in.*/
      else if (!RenderGraph::combineImageAccesses(it->second, access)) GraphicsInstance::showError("Use of the same attachment in different layouts within the same render pass is not supported. Use multiple render passes.");
    }
  }
  if (colorAttachmentOffset == imageAccesses.size()) colorAttachmentOffset = ~0U;
  else colorAttachmentCount = imageAccesses.size() - colorAttachmentOffset;
  inputAttachmentOffset = imageAccesses.size();
  for (const std::shared_ptr<Material>& material: materials) {
    /**@todo: Add automatic support for resolve attachments.*/
    const std::vector<std::pair<RenderGraph::ImageID, RenderGraph::ImageAccess>>& inputAttachmentAccesses = material->computeInputAttachmentAccesses();
    for (auto& [id, access]: inputAttachmentAccesses) {
      auto it = std::ranges::find(imageAccesses, id, &decltype(imageAccesses)::value_type::first);
      if (it == imageAccesses.end()) imageAccesses.emplace_back(id, access);
      else if (!RenderGraph::combineImageAccesses(it->second, access)) GraphicsInstance::showError("Use of the same attachment in different layouts within the same render pass is not supported. Use multiple render passes.");
    }
  }
  if (inputAttachmentOffset == imageAccesses.size()) inputAttachmentOffset = ~0U;
  else inputAttachmentCount = imageAccesses.size() - inputAttachmentOffset;
  boundImageOffset = imageAccesses.size();
  for (const std::shared_ptr<Material>& material: materials) {
    const std::vector<std::pair<RenderGraph::ImageID, RenderGraph::ImageAccess>>& boundImageAccesses = material->computeBoundImageAccesses();
    for (auto& [id, access]: boundImageAccesses) {
      auto it = std::ranges::find(imageAccesses, id, &decltype(imageAccesses)::value_type::first);
      if (it == imageAccesses.end()) imageAccesses.emplace_back(id, access);
      else if (!RenderGraph::combineImageAccesses(it->second, access)) GraphicsInstance::showError("Use of the same attachment in different layouts within the same render pass is not supported. Use multiple render passes.");
    }
  }
  if (boundImageOffset == imageAccesses.size()) boundImageOffset = ~0U;
  else boundImageCount = imageAccesses.size() - boundImageOffset;
}

VkRenderPass RenderPass::getRenderPass() const { return renderPass; }
std::shared_ptr<Framebuffer> RenderPass::getFramebuffer() const { return framebuffer; }
const std::unordered_map<std::shared_ptr<Material>, std::shared_ptr<Pipeline>>& RenderPass::getPipelines() { return pipelines; }
const RenderGraph& RenderPass::getGraph() const { return graph; }