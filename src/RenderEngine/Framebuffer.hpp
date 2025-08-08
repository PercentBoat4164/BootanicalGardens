#pragma once

#include <vulkan/vulkan_core.h>

#include <memory>
#include <vector>

class GraphicsDevice;
class Image;

class Framebuffer {
  GraphicsDevice* const device;
  std::vector<std::shared_ptr<Image>> images;
  VkFramebuffer framebuffer{VK_NULL_HANDLE};
  VkExtent2D extent;
  VkRect2D rect;

public:
  Framebuffer(GraphicsDevice* device, const std::vector<std::shared_ptr<Image>>& images, VkRenderPass renderPass);
  ~Framebuffer();

  [[nodiscard]] VkFramebuffer getFramebuffer() const;
  [[nodiscard]] VkExtent2D getExtent() const;
  [[nodiscard]] VkRect2D getRect() const;
  [[nodiscard]] const std::vector<std::shared_ptr<Image>>& getImages() const;
};
