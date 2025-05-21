#include "Image.hpp"

#include "Buffer.hpp"
#include "GraphicsDevice.hpp"
#include "GraphicsInstance.hpp"

#include <vma/vk_mem_alloc.h>
#include <volk/volk.h>

#include <utility>

Image::Image(const std::shared_ptr<GraphicsDevice>& device, std::string name, VkImage image, const VkFormat format, const VkExtent3D extent, const VkImageUsageFlags usage, uint32_t mipLevels, VkSampleCountFlagBits sampleCount, VkImageView view) : Resource(Resource::Image, device), _name(std::move(name)), _shouldDestroy(false), _image(image), _format(format), _aspect(aspectFromFormat(format)), _extent(extent), _usage(usage), _view(view), _mipLevels(mipLevels), _sampleCount(sampleCount) {}

Image::Image(const std::shared_ptr<GraphicsDevice>& device, std::string name, const VkFormat format, const VkExtent3D extent, const VkImageUsageFlags usage, const uint32_t mipLevels, const VkSampleCountFlagBits sampleCount) : Resource(Resource::Image, device), _name(std::move(name)), _format(format), _aspect(aspectFromFormat(format)), _extent(extent), _usage(usage), _view(VK_NULL_HANDLE), _mipLevels(mipLevels), _sampleCount(sampleCount) {
  const VkImageCreateInfo imageCreateInfo {
    .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .pNext         = nullptr,
    .flags         = 0,
    .imageType     = VK_IMAGE_TYPE_2D,
    .format        = _format,
    .extent        = _extent,
    .mipLevels     = _mipLevels,
    .arrayLayers   = 1,
    .samples       = _sampleCount,
    .tiling        = VK_IMAGE_TILING_OPTIMAL,
    .usage         = _usage,
    .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
  };
  const VmaAllocationCreateInfo allocationCreateInfo {
    .usage     = VMA_MEMORY_USAGE_GPU_ONLY,
    .pUserData = this,
  };
  if (const VkResult result = vmaCreateImage(device->allocator, &imageCreateInfo, &allocationCreateInfo, &_image, &allocation, nullptr); result != VK_SUCCESS) GraphicsInstance::showError(result, "Failed to create image");
  if (_image == VK_NULL_HANDLE) return;  /**@todo: Find out why (check device supported formats and limits), then report the error.**/
  if (!_name.empty()) vmaSetAllocationName(device->allocator, allocation, _name.c_str());
  const VkImageViewCreateInfo imageViewCreateInfo {
    .sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .pNext      = nullptr,
    .flags      = 0,
    .image      = _image,
    .viewType   = VK_IMAGE_VIEW_TYPE_2D,
    .format     = _format,
    .components = {
      .r = VK_COMPONENT_SWIZZLE_IDENTITY,
      .g = VK_COMPONENT_SWIZZLE_IDENTITY,
      .b = VK_COMPONENT_SWIZZLE_IDENTITY,
      .a = VK_COMPONENT_SWIZZLE_IDENTITY
  },
  .subresourceRange = {
    .aspectMask = _aspect,
    .baseMipLevel = 0,
    .levelCount = _mipLevels,
    .baseArrayLayer = 0,
    .layerCount = _layerCount
  }
  };
  if (const VkResult result = vkCreateImageView(device->device, &imageViewCreateInfo, nullptr, &_view); result != VK_SUCCESS) GraphicsInstance::showError(result, "Failed to create image view");
}

Image::~Image() {
  vkDestroyImageView(device->device, _view, nullptr);
  _view = VK_NULL_HANDLE;
  if (_shouldDestroy) vmaDestroyImage(device->allocator, _image, allocation);
  _image     = VK_NULL_HANDLE;
  allocation = VK_NULL_HANDLE;
}

void Image::rebuild(VkExtent3D newExtent) {
  std::string name = _name;
  VkFormat format = _format;
  VkImageUsageFlags usage = _usage;
  uint32_t mipLevels = _mipLevels;
  VkSampleCountFlagBits sampleCount = _sampleCount;
  std::destroy_at(this);
  std::construct_at(this, device, name, format, newExtent, usage, mipLevels, sampleCount);
}

VkImage Image::image() const {
  return _image;
}

VkExtent3D Image::extent() const {
  return _extent;
}

VkImageView Image::view() const {
  return _view;
}

VkImageView Image::view(const VkComponentMapping mapping, const VkImageSubresourceRange& subresourceRange) const {
    const VkImageViewCreateInfo imageViewCreateInfo {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext            = nullptr,
        .flags            = 0,
        .image            = _image,
        .viewType         = VK_IMAGE_VIEW_TYPE_2D,
        .format           = _format,
        .components       = mapping,
        .subresourceRange = subresourceRange
    };
    VkImageView view;
    GraphicsInstance::showError(vkCreateImageView(device->device, &imageViewCreateInfo, nullptr, &view), "Failed to create image view");
    return view;
}

VkFormat Image::format() const {
  return _format;
}

VkImageAspectFlags Image::aspect() const {
    return _aspect;
}

uint32_t Image::mipLevels() const {
  return _mipLevels;
}

uint32_t Image::layerCount() const {
  return _layerCount;
}

VkSampleCountFlagBits Image::sampleCount() const {
  return _sampleCount;
}

VkImageSubresourceRange Image::wholeRange() const {
  return {
    .aspectMask = _aspect,
    .baseMipLevel = 0,
    .levelCount = _mipLevels,
    .baseArrayLayer = 0,
    .layerCount = _layerCount
  };
}

void* Image::getObject() const {
  return reinterpret_cast<void*>(_image);
}

void* Image::getView() const {
  return reinterpret_cast<void*>(_view);
}
