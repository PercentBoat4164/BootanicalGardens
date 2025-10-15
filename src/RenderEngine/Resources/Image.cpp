#include "Image.hpp"

#include "src/RenderEngine/Resources/Buffer.hpp"
#include "src/RenderEngine/GraphicsDevice.hpp"
#include "src/RenderEngine/GraphicsInstance.hpp"
#include "src/Tools/Hashing.hpp"

#include <vma/vk_mem_alloc.h>
#include <volk/volk.h>

#include <utility>

Image::Image(GraphicsDevice* const device, std::string name, VkImage image, const VkFormat format, const VkExtent3D extent, const VkImageUsageFlags usage, const uint32_t mipLevels, const VkSampleCountFlags sampleCount, VkImageView view) : Resource(Resource::Image, device), _name(std::move(name)), _shouldDestroy(false), _image(image), _format(format), _aspect(aspectFromFormat(format)), _extent(extent), _usage(usage), _view(view), _mipLevels(mipLevels), _sampleCount(sampleCount) {
#if VK_EXT_debug_utils & BOOTANICAL_GARDENS_ENABLE_VULKAN_DEBUG_UTILS
  if (GraphicsInstance::extensionEnabled(Tools::hash(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))) {
    VkDebugUtilsObjectNameInfoEXT nameInfo {
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
      .pNext = nullptr,
      .objectType = VK_OBJECT_TYPE_IMAGE,
      .objectHandle = reinterpret_cast<uint64_t>(_image),
      .pObjectName = _name.c_str()
    };
    if (const VkResult result = vkSetDebugUtilsObjectNameEXT(device->device, &nameInfo); result != VK_SUCCESS) GraphicsInstance::showError(result, "failed to set debug utils object name");
    const std::string viewName = _name + " View";
    nameInfo.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
    nameInfo.objectHandle = reinterpret_cast<uint64_t>(_view);
    nameInfo.pObjectName = viewName.c_str();
    if (const VkResult result = vkSetDebugUtilsObjectNameEXT(device->device, &nameInfo); result != VK_SUCCESS) GraphicsInstance::showError(result, "failed to set debug utils object name");
  }
#endif
}

Image::Image(GraphicsDevice* const device, std::string name, const VkFormat format, const VkExtent3D extent, const VkImageUsageFlags usage, const uint32_t mipLevels, const VkSampleCountFlags sampleCount) : Resource(Resource::Image, device), _name(std::move(name)), _format(format), _aspect(aspectFromFormat(format)), _extent(extent), _usage(usage), _view(VK_NULL_HANDLE), _mipLevels(mipLevels), _sampleCount(sampleCount) {
  const VkImageCreateInfo imageCreateInfo {
    .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .pNext         = nullptr,
    .flags         = 0,
    .imageType     = VK_IMAGE_TYPE_2D,
    .format        = _format,
    .extent        = _extent,
    .mipLevels     = _mipLevels,
    .arrayLayers   = 1,
    .samples       = static_cast<VkSampleCountFlagBits>(_sampleCount),
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
#if VK_EXT_debug_utils & BOOTANICAL_GARDENS_ENABLE_VULKAN_DEBUG_UTILS
  if (GraphicsInstance::extensionEnabled(Tools::hash(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))) {
    const VkDebugUtilsObjectNameInfoEXT nameInfo {
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
      .pNext = nullptr,
      .objectType = VK_OBJECT_TYPE_IMAGE,
      .objectHandle = reinterpret_cast<uint64_t>(_image),
      .pObjectName = _name.c_str()
    };
    if (const VkResult result = vkSetDebugUtilsObjectNameEXT(device->device, &nameInfo); result != VK_SUCCESS) GraphicsInstance::showError(result, "failed to set debug utils object name");
  }
#endif
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
#if VK_EXT_debug_utils & BOOTANICAL_GARDENS_ENABLE_VULKAN_DEBUG_UTILS
  if (GraphicsInstance::extensionEnabled(Tools::hash(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))) {
    const std::string viewName = _name + " View";
    const VkDebugUtilsObjectNameInfoEXT nameInfo {
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
      .pNext = nullptr,
      .objectType = VK_OBJECT_TYPE_IMAGE_VIEW,
      .objectHandle = reinterpret_cast<uint64_t>(_view),
      .pObjectName = viewName.c_str()
    };
    if (const VkResult result = vkSetDebugUtilsObjectNameEXT(device->device, &nameInfo); result != VK_SUCCESS) GraphicsInstance::showError(result, "failed to set debug utils object name");
  }
#endif
}

Image::~Image() {
  vkDestroyImageView(device->device, _view, nullptr);
  _view = VK_NULL_HANDLE;
  if (_shouldDestroy) vmaDestroyImage(device->allocator, _image, allocation);
  _image     = VK_NULL_HANDLE;
  allocation = VK_NULL_HANDLE;
}

void Image::rebuild(const VkExtent3D newExtent, const VkSampleCountFlags newSampleCount) {
  std::string name = _name;
  VkFormat format = _format;
  VkImageUsageFlags usage = _usage;
  uint32_t mipLevels = _mipLevels;
  VkSampleCountFlags sampleCount = newSampleCount == VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM ? _sampleCount : newSampleCount;
  VkExtent3D extent = newExtent.width == 0 || newExtent.height == 0 || newExtent.depth == 0 ? _extent : newExtent;
  std::destroy_at(this);
  std::construct_at(this, device, name, format, extent, usage, mipLevels, sampleCount);
}

VkImage Image::getImage() const {
  return _image;
}

VkExtent3D Image::getExtent() const {
  return _extent;
}

VkImageView Image::getImageView() const {
  return _view;
}

VkImageView Image::getImageView(const VkComponentMapping mapping, const VkImageSubresourceRange& subresourceRange) const {
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

VkFormat Image::getFormat() const {
  return _format;
}

VkImageAspectFlags Image::getAspect() const {
    return _aspect;
}

uint32_t Image::getMipLevels() const {
  return _mipLevels;
}

uint32_t Image::getLayerCount() const {
  return _layerCount;
}

VkSampleCountFlags Image::getSampleCount() const {
  return _sampleCount;
}

VkImageSubresourceRange Image::getWholeRange() const {
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
