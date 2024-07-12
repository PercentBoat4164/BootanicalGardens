#pragma once
#include <volk.h>

#include <vector>

class Pipeline;
class GraphicsDevice;
class Image;

class RenderPass {
  const GraphicsDevice& device;
public:
  VkRenderPass renderPass{VK_NULL_HANDLE};
  std::vector<VkAttachmentDescription> attachmentDescriptions;
  std::vector<VkFramebuffer> framebuffers;
  std::vector<Pipeline*> pipelines;

  RenderPass(const GraphicsDevice& device, const std::vector<Pipeline*>& pipelines);
  ~RenderPass();

  void render(VkCommandBuffer commandBuffer, VkRect2D area, const std::vector<VkClearValue>& clearValues, uint32_t swapchainIndex) const;
  [[nodiscard]] bool isCompatibleWith(const RenderPass& other) const;
};
