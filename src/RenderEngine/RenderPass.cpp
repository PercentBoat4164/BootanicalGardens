#include "RenderPass.hpp"

#include "GraphicsDevice.hpp"
// #include "Image.hpp"

#include "Pipeline.hpp"

#include <chrono>
#include <cmath>
#include <memory>

RenderPass::RenderPass(const GraphicsDevice& device, const std::vector<Pipeline*>& pipelines) : device(device), pipelines(pipelines) {
  // attachmentDescriptions.push_back({
  //   ._format = swapchainImages.back()._format,
  //   .samples = VK_SAMPLE_COUNT_1_BIT,
  //   .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
  //   .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
  //   .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
  //   .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
  //   .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
  //   .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
  // });
  // constexpr VkAttachmentReference attachmentReference {
  //   .attachment = 0,
  //   .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
  // };
  // const VkSubpassDescription subpassDescription {
  //   .flags = 0,
  //   .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
  //   .inputAttachmentCount = 0,
  //   .pInputAttachments = nullptr,
  //   .colorAttachmentCount = 1,
  //   .pColorAttachments = &attachmentReference,
  //   .pResolveAttachments = nullptr,
  //   .pDepthStencilAttachment = nullptr,
  //   .preserveAttachmentCount = 0,
  //   .pPreserveAttachments = nullptr
  // };
  // const VkRenderPassCreateInfo renderPassCreateInfo {
  //   .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
  //   .pNext = nullptr,
  //   .flags = 0,
  //   .attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size()),
  //   .pAttachments = attachmentDescriptions.data(),
  //   .subpassCount = 1,
  //   .pSubpasses = &subpassDescription,
  //   .dependencyCount = 0,
  //   .pDependencies = nullptr
  // };
  // vkCreateRenderPass(device.device, &renderPassCreateInfo, nullptr, &renderPass);
  // VkFramebufferCreateInfo framebufferCreateInfo {
  //   .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
  //   .pNext = nullptr,
  //   .flags = 0,
  //   .renderPass = renderPass,
  //   .attachmentCount = 1,
  //   .width = swapchainImages.back()._extent.width,
  //   .height = swapchainImages.back()._extent.height,
  //   .layers = 1
  // };
  // framebuffers.reserve(swapchainImages.size());
  // for (const Image& swapchainImage : swapchainImages) {
  //   framebufferCreateInfo.pAttachments = &swapchainImage._view;
  //   VkFramebuffer framebuffer;
  //   vkCreateFramebuffer(device.device, &framebufferCreateInfo, nullptr, &framebuffer);
  //   framebuffers.push_back(framebuffer);
  // }
}

RenderPass::~RenderPass() {
  for (VkFramebuffer& framebuffer : framebuffers) {
    if (framebuffer != VK_NULL_HANDLE) {
      vkDestroyFramebuffer(device.device, framebuffer, nullptr);
      framebuffer = VK_NULL_HANDLE;
    }
  }
  if (renderPass != VK_NULL_HANDLE) {
    vkDestroyRenderPass(device.device, renderPass, nullptr);
    renderPass = VK_NULL_HANDLE;
  }
}

void RenderPass::render(VkCommandBuffer commandBuffer, const VkRect2D area, const std::vector<VkClearValue>& clearValues, const uint32_t swapchainIndex) const {
  if (renderPass != VK_NULL_HANDLE) {
    const VkRenderPassBeginInfo renderPassBeginInfo {
      .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .pNext           = nullptr,
      .renderPass      = renderPass,
      .framebuffer     = framebuffers[swapchainIndex],
      .renderArea      = area,
      .clearValueCount = static_cast<uint32_t>(clearValues.size()),
      .pClearValues    = clearValues.data()
    };
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
  }
  for (const Pipeline* pipeline : pipelines) pipeline->execute(commandBuffer, std::ceil(area.extent.width / 16.0), std::ceil(area.extent.height / 16.0), 1);
  if (renderPass != VK_NULL_HANDLE)
    vkCmdEndRenderPass(commandBuffer);
}

bool RenderPass::isCompatibleWith(const RenderPass& other) const {
  if (attachmentDescriptions.size() != other.attachmentDescriptions.size()) return false;
  for (uint32_t i{}; i < attachmentDescriptions.size(); ++i) {
    const VkAttachmentDescription& desc      = attachmentDescriptions[i];
    const VkAttachmentDescription& otherDesc = other.attachmentDescriptions[i];
    if (desc.format != otherDesc.format || desc.samples != otherDesc.samples) return false;
  }
  return true;
}