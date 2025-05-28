#include "Buffer.hpp"

#include "src/RenderEngine/Resources/Image.hpp"
#include "src/RenderEngine/GraphicsDevice.hpp"
#include "src/RenderEngine/GraphicsInstance.hpp"

Buffer::BufferMapping::BufferMapping(const std::shared_ptr<const GraphicsDevice>& device, const std::shared_ptr<const Buffer>& buffer) : device(device), buffer(buffer){
  vmaMapMemory(device->allocator, buffer->allocation, &data);
}

Buffer::BufferMapping::~BufferMapping() {
  vmaUnmapMemory(device->allocator, buffer->allocation);
}

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

std::shared_ptr<Buffer::BufferMapping> Buffer::map() {
  if (!mapping.expired()) return mapping.lock();
  auto newMapping = std::make_shared<BufferMapping>(device, std::dynamic_pointer_cast<Buffer>(shared_from_this()));
  mapping = newMapping;
  return newMapping;
}

void* Buffer::getObject() const {
  return buffer;
}

void* Buffer::getView() const {
  return view;
}
