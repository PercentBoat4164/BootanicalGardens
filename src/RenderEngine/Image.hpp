#pragma once

#include "Resource.hpp"

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include <glm/vec2.hpp>
#include <string>

class GraphicsDevice;

class Image : public Resource {
  const GraphicsDevice& device;
  VmaAllocation allocation{VK_NULL_HANDLE};

  std::string _name;
  bool _shouldDestroy;
  VkImage _image;
  VkFormat _format;
  VkExtent2D _extent;
  VkImageUsageFlags _usage;
  VkImageView _view;

public:
  explicit Image(const GraphicsDevice& device, std::string name);
  Image(const GraphicsDevice& device, std::string name, VkImage image, VkFormat format, VkExtent2D extent, VkImageUsageFlags usage, VkImageView view);
  Image(const GraphicsDevice& device, std::string name, VkFormat format, VkExtent2D extent, VkImageUsageFlags usage, uint32_t mipLevels=1, VkSampleCountFlagBits samples=VK_SAMPLE_COUNT_1_BIT);
  Image(const Image&) = default;
  Image(Image&&) = default;
  ~Image() override;

  void buildInPlace(VkFormat format, VkExtent2D extent, VkImageUsageFlags usage, uint32_t mipLevels=1, VkSampleCountFlagBits samples=VK_SAMPLE_COUNT_1_BIT);

  [[nodiscard]] VkImage image() const;
  [[nodiscard]] VkExtent2D extent() const;
  [[nodiscard]] VkImageView view() const;
  [[nodiscard]] VkSampler sampler() const;

private:
  [[nodiscard]] void* getObject() const override;
  [[nodiscard]] void* getView() const override;
};
