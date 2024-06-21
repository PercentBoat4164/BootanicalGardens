#include "Image.hpp"

#include "GraphicsDevice.hpp"
#include "GraphicsInstance.hpp"

#include <vk_mem_alloc.h>
#include <volk.h>

#include <utility>

Image::Image(const GraphicsDevice& device) : device(device), image(VK_NULL_HANDLE), format(VK_FORMAT_UNDEFINED), extent(), usage(), view(VK_NULL_HANDLE){};

Image::Image(const GraphicsDevice& device, std::string name, VkImage image, const VkFormat format, const VkExtent2D extent, const VkImageUsageFlags usage, VkImageView view) : device(device), name(std::move(name)), image(image), format(format), extent(extent), usage(usage), view(view) {}

Image::Image(const GraphicsDevice& device, const std::string& name, const VkFormat format, const VkExtent2D extent, const VkImageUsageFlags usage, const uint32_t mipLevels, const VkSampleCountFlagBits samples) : device(device), name(name), image(VK_NULL_HANDLE), format(format), extent(extent), usage(usage), view(VK_NULL_HANDLE) {
  const VkImageCreateInfo imageCreateInfo{
      .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .pNext         = nullptr,
      .flags         = 0,
      .imageType     = VK_IMAGE_TYPE_2D,
      .format        = format,
      .extent        = {extent.width, extent.height, 1},
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
  GraphicsInstance::showError(vmaCreateImage(device.allocator, &imageCreateInfo, &allocationCreateInfo, &image, &allocation, nullptr), "Failed to create image.");
  vmaSetAllocationName(device.allocator, allocation, name.c_str());
  const VkImageViewCreateInfo imageViewCreateInfo{
      .sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .pNext      = nullptr,
      .flags      = 0,
      .image      = image,
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
  GraphicsInstance::showError(vkCreateImageView(device.device, &imageViewCreateInfo, nullptr, &view), "Failed to create image view.");
}

Image& Image::operator=(const Image& other) {
  if (this != &other && device.device == other.device.device) {
    allocation = other.allocation;
    name       = other.name;
    image      = other.image;
    format     = other.format;
    extent     = other.extent;
    usage      = other.usage;
    view       = other.view;
  }
  return *this;
}

Image::~Image() {
  if (view != VK_NULL_HANDLE) {
    vkDestroyImageView(device.device, view, nullptr);
    view = VK_NULL_HANDLE;
  }
  if (allocation == VK_NULL_HANDLE && image != VK_NULL_HANDLE) {
    vkDestroyImage(device.device, image, nullptr);
    image = VK_NULL_HANDLE;
  }
  if (allocation != VK_NULL_HANDLE) {
    vmaDestroyImage(device.allocator, image, allocation);
    image = VK_NULL_HANDLE;
    allocation = VK_NULL_HANDLE;
  }
}
void* Image::getObject() {
  return image;
}