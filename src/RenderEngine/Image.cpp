#include "Image.hpp"

#include "Buffer.hpp"
#include "GraphicsDevice.hpp"
#include "GraphicsInstance.hpp"

#include <vk_mem_alloc.h>
#include <volk.h>

#include <utility>

Image::Image(const GraphicsDevice& device, std::string name, VkImage image, const VkFormat format, const VkExtent3D extent, const VkImageUsageFlags usage, VkImageView view) : Resource(Resource::Image, device), _name(std::move(name)), _shouldDestroy(false), _image(image), _format(format), _aspect(Image::aspectFromFormat(format)), _extent(extent), _usage(usage), _view(view) {}

Image::Image(const GraphicsDevice& device, std::string name, const VkFormat format, const VkExtent3D extent, const VkImageUsageFlags usage, const uint32_t mipLevels, const VkSampleCountFlagBits samples) : Resource(Resource::Image, device), _name(std::move(name)), _format(format), _aspect(Image::aspectFromFormat(format)), _extent(extent), _usage(usage), _view(VK_NULL_HANDLE) {
  buildInPlace(format, extent, usage, mipLevels, samples);
}

Image::~Image() {
  vkDestroyImageView(device.device, _view, nullptr);
  _view = VK_NULL_HANDLE;
  if (_shouldDestroy) vmaDestroyImage(device.allocator, _image, allocation);
  _image     = VK_NULL_HANDLE;
  allocation = VK_NULL_HANDLE;
}

void Image::buildInPlace(const VkFormat format, const VkExtent3D extent, const VkImageUsageFlags usage, const uint32_t mipLevels, const VkSampleCountFlagBits samples) {
  _format = format;
  _extent = VkExtent3D(extent.width, extent.height, extent.depth);
  _usage = usage;
  _mipLevels = mipLevels;
  const VkImageCreateInfo imageCreateInfo{
      .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .pNext         = nullptr,
      .flags         = 0,
      .imageType     = VK_IMAGE_TYPE_2D,
      .format        = format,
      .extent        = _extent,
      .mipLevels     = mipLevels,
      .arrayLayers   = 1,
      .samples       = samples,
      .tiling        = VK_IMAGE_TILING_OPTIMAL,
      .usage         = usage,
      .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
  };
  const VmaAllocationCreateInfo allocationCreateInfo{
      .usage     = VMA_MEMORY_USAGE_GPU_ONLY,
      .pUserData = this,
  };
  GraphicsInstance::showError(vmaCreateImage(device.allocator, &imageCreateInfo, &allocationCreateInfo, &_image, &allocation, nullptr), "Failed to create image.");
  vmaSetAllocationName(device.allocator, allocation, _name.c_str());
  const VkImageViewCreateInfo imageViewCreateInfo {
      .sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .pNext      = nullptr,
      .flags      = 0,
      .image      = _image,
      .viewType   = VK_IMAGE_VIEW_TYPE_2D,
      .format     = format,
      .components = {
          .r = VK_COMPONENT_SWIZZLE_IDENTITY,
          .g = VK_COMPONENT_SWIZZLE_IDENTITY,
          .b = VK_COMPONENT_SWIZZLE_IDENTITY,
          .a = VK_COMPONENT_SWIZZLE_IDENTITY
      },
      .subresourceRange = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .baseMipLevel = 0,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 1
      }
  };
  GraphicsInstance::showError(vkCreateImageView(device.device, &imageViewCreateInfo, nullptr, &_view), "Failed to create image view.");
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
    GraphicsInstance::showError(vkCreateImageView(device.device, &imageViewCreateInfo, nullptr, &view), "Failed to create image view.");
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

void* Image::getObject() const {
  return _image;
}

void* Image::getView() const {
  return _view;
}
