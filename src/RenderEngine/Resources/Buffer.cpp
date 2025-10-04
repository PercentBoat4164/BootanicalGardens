#include "Buffer.hpp"

#include "src/RenderEngine/Resources/Image.hpp"
#include "src/RenderEngine/GraphicsDevice.hpp"
#include "src/RenderEngine/GraphicsInstance.hpp"

#include <volk/volk.h>

Buffer::BufferMapping::BufferMapping(GraphicsDevice* const device, const std::shared_ptr<const Buffer>& buffer) : device(device), buffer(buffer){
  vmaMapMemory(device->allocator, buffer->allocation, &data);
}

Buffer::BufferMapping::~BufferMapping() {
  vmaUnmapMemory(device->allocator, buffer->allocation);
}

Buffer::Buffer(GraphicsDevice* const device, const char* name, const VkDeviceSize bufferSize, const VkBufferUsageFlags usage, const VkMemoryPropertyFlags required, const VkMemoryPropertyFlags preferred, const VmaMemoryUsage memoryUsage, const VmaAllocationCreateFlags flags) :
    Resource(Resource::Buffer, device),
    size(bufferSize) {
  const VkBufferCreateInfo bufferCreateInfo{
      .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext                 = nullptr,
      .flags                 = 0,
      .size                  = bufferSize,
      .usage                 = usage,
      .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 0,
      .pQueueFamilyIndices   = nullptr
  };
  const VmaAllocationCreateInfo allocationCreateInfo {
      .flags = VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT | flags,
      .usage = memoryUsage,
      .requiredFlags = required,
      .preferredFlags = preferred,
      .memoryTypeBits = 0,
      .pool = VK_NULL_HANDLE,
      .pUserData = this,
      .priority = 1.0f
  };

  if (const VkResult result = vmaCreateBuffer(device->allocator, &bufferCreateInfo, &allocationCreateInfo, &buffer, &allocation, &allocationInfo); result != VK_SUCCESS) GraphicsInstance::showError(result, name);
  vmaSetAllocationName(device->allocator, allocation, name);
#if VK_EXT_debug_utils & BOOTANICAL_GARDENS_ENABLE_VULKAN_DEBUG_UTILS
  if (GraphicsInstance::extensionEnabled(Tools::hash(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))) {
    const VkDebugUtilsObjectNameInfoEXT nameInfo {
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
      .pNext = nullptr,
      .objectType = VK_OBJECT_TYPE_BUFFER,
      .objectHandle = std::bit_cast<uint64_t>(buffer),
      .pObjectName = name
    };
    if (const VkResult result = vkSetDebugUtilsObjectNameEXT(device->device, &nameInfo); result != VK_SUCCESS) GraphicsInstance::showError(result, "failed to set debug utils object name");
  }
#endif
}

Buffer::~Buffer() {
  vmaDestroyBuffer(device->allocator, buffer, allocation);
  buffer     = VK_NULL_HANDLE;
  allocation = VK_NULL_HANDLE;
}

VkBuffer Buffer::getBuffer() const {
  return buffer;
}

VkDeviceSize Buffer::getSize() const {
  return size;
}

std::shared_ptr<Buffer::BufferMapping> Buffer::map() {
  if (!mapping.expired()) return mapping.lock();
  auto newMapping = std::make_shared<BufferMapping>(device, std::dynamic_pointer_cast<Buffer>(shared_from_this()));
  mapping = newMapping;
  return newMapping;
}

void* Buffer::getObject() const {
  return reinterpret_cast<void *>(buffer);
}

void* Buffer::getView() const {
  return reinterpret_cast<void *>(view);
}
