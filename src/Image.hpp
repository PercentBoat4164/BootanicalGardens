#pragma once

#include "Resource.hpp"

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include <string>

class GraphicsDevice;

class Image : public Resource {
  const GraphicsDevice& device;
  VmaAllocation allocation{VK_NULL_HANDLE};

public:
  std::string name;
  VkImage image;
  VkFormat format;
  VkExtent2D extent;
  VkImageUsageFlags usage;
  VkImageView view;

  explicit Image(const GraphicsDevice& device);
  Image(const GraphicsDevice& device, std::string name, VkImage image, VkFormat format, VkExtent2D extent, VkImageUsageFlags usage, VkImageView view);
  Image(const GraphicsDevice& device, const std::string& name, VkFormat format, VkExtent2D extent, VkImageUsageFlags usage, uint32_t mipLevels=1, VkSampleCountFlagBits samples=VK_SAMPLE_COUNT_1_BIT);
  Image(const Image&) = default;
  Image(Image&&) = default;
  Image& operator=(const Image& other);

  ~Image() override;

  void* getObject() override;
};
