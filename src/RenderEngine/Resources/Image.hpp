#pragma once

#include "Resource.hpp"

#include <vma/vk_mem_alloc.h>

#include <string>
#include <vulkan/utility/vk_format_utils.h>

class GraphicsDevice;
class Buffer;

class Image : public Resource {
  VmaAllocation allocation{VK_NULL_HANDLE};

  std::string _name;
  bool _shouldDestroy{true};
  VkImage _image{VK_NULL_HANDLE};
  VkFormat _format;
  VkImageAspectFlags _aspect;
  VkExtent3D _extent;
  VkImageUsageFlags _usage;
  VkImageView _view;
  uint32_t _mipLevels{1};
  uint32_t _layerCount{1};
  VkSampleCountFlagBits _sampleCount{VK_SAMPLE_COUNT_1_BIT};

public:
  enum Usages : VkImageUsageFlags {
    Texture = VK_IMAGE_USAGE_SAMPLED_BIT,
    Compute = VK_IMAGE_USAGE_STORAGE_BIT,
  };

  Image(const std::shared_ptr<GraphicsDevice>& device, std::string name, VkImage image, VkFormat format, VkExtent3D extent, VkImageUsageFlags usage=0, uint32_t mipLevels=1, VkSampleCountFlagBits sampleCount=VK_SAMPLE_COUNT_1_BIT, VkImageView view=VK_NULL_HANDLE);
  Image(const std::shared_ptr<GraphicsDevice>& device, std::string name, VkFormat format, VkExtent3D extent, VkImageUsageFlags usage, uint32_t mipLevels=1, VkSampleCountFlagBits sampleCount=VK_SAMPLE_COUNT_1_BIT);
  ~Image() override;

  void rebuild(VkExtent3D newExtent);

  [[nodiscard]] VkImage getImage() const;
  [[nodiscard]] VkExtent3D getExtent() const;
  [[nodiscard]] VkImageView getImageView() const;
  [[nodiscard]] VkImageView getImageView(VkComponentMapping mapping, const VkImageSubresourceRange&subresourceRange) const;
  [[nodiscard]] VkFormat getFormat() const;
  [[nodiscard]] VkImageAspectFlags getAspect() const;
  [[nodiscard]] uint32_t getMipLevels() const;
  [[nodiscard]] uint32_t getLayerCount() const;
  [[nodiscard]] VkSampleCountFlagBits getSampleCount() const;
  [[nodiscard]] VkImageSubresourceRange getWholeRange() const;

private:
  [[nodiscard]] void* getObject() const override;
  [[nodiscard]] void* getView() const override;

  static VkImageAspectFlags aspectFromFormat(const VkFormat format) {
    return (vkuFormatIsDepthOnly(format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_NONE) |
           (vkuFormatIsDepthAndStencil(format) ? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT : VK_IMAGE_ASPECT_NONE) |
           (vkuFormatIsColor(format) ? VK_IMAGE_ASPECT_COLOR_BIT: VK_IMAGE_ASPECT_NONE);
  }
};
