#pragma once

#include "Resource.hpp"

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include <glm/vec2.hpp>
#include <string>
#include <vector>

class GraphicsDevice;
class Buffer;

class Image : public Resource {
  const GraphicsDevice& device;
  VmaAllocation allocation{VK_NULL_HANDLE};

  std::string _name;
  bool _shouldDestroy;
  VkImage _image;
  VkFormat _format;
  VkExtent3D _extent;
  VkImageUsageFlags _usage;
  VkImageView _view;

public:
  enum Usages : VkImageUsageFlags {
    Texture = VK_IMAGE_USAGE_SAMPLED_BIT,
    Compute = VK_IMAGE_USAGE_STORAGE_BIT,
  };

  Image(const GraphicsDevice& device, std::string name);
  Image(const GraphicsDevice& device, std::string name, VkImage image, VkFormat format, VkExtent3D extent, VkImageUsageFlags usage, VkImageView view);
  Image(const GraphicsDevice& device, std::string name, VkFormat format, VkExtent3D extent, VkImageUsageFlags usage, uint32_t mipLevels=1, VkSampleCountFlagBits samples=VK_SAMPLE_COUNT_1_BIT);
  Image(const GraphicsDevice& device, std::string name, std::vector<std::byte> bytes, VkFormat format, VkExtent3D extent, VkImageUsageFlags usage, uint32_t mipLevels=1);
  Image(const GraphicsDevice& device, std::string name, class Buffer& buffer);
  Image(const Image&) = default;
  Image(Image&&) = default;
  ~Image() override;

  void buildInPlace(VkFormat format, VkExtent3D extent, VkImageUsageFlags usage, uint32_t mipLevels = 1, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);

  void transitionToLayout(VkImageLayout oldLayout, VkImageLayout newLayout) const;

  [[nodiscard]] VkImage image() const;
  [[nodiscard]] VkExtent3D extent() const;
  [[nodiscard]] VkImageView view() const;
  [[nodiscard]] VkImageView view(VkComponentMapping mapping, const VkImageSubresourceRange&subresourceRange) const;
  [[nodiscard]] VkFormat format() const;

private:
  [[nodiscard]] void* getObject() const override;
  [[nodiscard]] void* getView() const override;
};
