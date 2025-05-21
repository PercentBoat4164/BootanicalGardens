#include "Buffer.hpp"

#include "src/RenderEngine/GraphicsDevice.hpp"
#include "src/RenderEngine/GraphicsInstance.hpp"
#include "src/RenderEngine/Image.hpp"

Buffer::Buffer(const std::shared_ptr<GraphicsDevice>& device, const char* name, const VkDeviceSize bufferSize, const VkBufferUsageFlags usage, const VkMemoryPropertyFlags required, const VkMemoryPropertyFlags preferred, const VmaMemoryUsage memoryUsage, const VmaAllocationCreateFlags flags) :
    Resource(Resource::Buffer, device) {
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
  return allocationInfo.size;
}

void* Buffer::getObject() const {
  return buffer;
}

void* Buffer::getView() const {
  return view;
}
