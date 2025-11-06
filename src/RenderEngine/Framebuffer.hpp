#pragma once

#include <vulkan/vulkan_core.h>

#include <memory>
#include <vector>

class GraphicsDevice;
class Image;

class Framebuffer {
  GraphicsDevice* const device;
  std::vector<const Image*> images;
  VkFramebuffer framebuffer{VK_NULL_HANDLE};
  VkExtent2D extent;
  VkRect2D rect;

public:
  Framebuffer(GraphicsDevice* device, const std::vector<const Image*>& images, VkRenderPass renderPass);
  ~Framebuffer();

  [[nodiscard]] VkFramebuffer getFramebuffer() const;
  [[nodiscard]] VkExtent2D getExtent() const;
  [[nodiscard]] VkRect2D getRect() const;
  [[nodiscard]] std::vector<const Image*> getImages() const;
};
