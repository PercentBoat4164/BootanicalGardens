#include "Buffer.hpp"

#include "Image.hpp"

Buffer::Buffer(const GraphicsDevice& device, const std::string& name, const std::size_t size, const VkBufferUsageFlags usage, const VmaMemoryUsage memoryUsage) : Resource(Resource::Buffer, device) {
  const VkBufferCreateInfo bufferCreateInfo{
      .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext                 = nullptr,
      .flags                 = 0,
      .size                  = size,
      .usage                 = usage,
      .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 0,
      .pQueueFamilyIndices   = nullptr
  };
  const VmaAllocationCreateInfo allocationCreateInfo {
      .usage = memoryUsage,
      .pUserData = this,
  };
  GraphicsInstance::showError(vmaCreateBuffer(device.allocator, &bufferCreateInfo, &allocationCreateInfo, &_buffer, &allocation, &allocationInfo), "Failed to create buffer [" + name + "].");
  vmaSetAllocationName(device.allocator, allocation, name.c_str());
}

Buffer::~Buffer() {
  vmaDestroyBuffer(device.allocator, _buffer, allocation);
  _buffer     = VK_NULL_HANDLE;
  allocation = VK_NULL_HANDLE;
}

VkBuffer Buffer::buffer() const {
  return _buffer;
}

uint64_t Buffer::size() const {
  return allocationInfo.size;
}

void* Buffer::getObject() const {
  return _buffer;
}

void* Buffer::getView() const {
  return _view;
}
