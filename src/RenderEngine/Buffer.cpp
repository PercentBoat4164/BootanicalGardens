#include "Buffer.hpp"

#include <volk.h>

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

void Buffer::copyTo(const class Image* image, const VkImageLayout imageLayout) const {
  VkCommandBuffer commandBuffer = device.getOneShotCommandBuffer();
  const VkBufferImageCopy wholeRegion {
    .bufferOffset      = 0,
    .bufferRowLength   = 0,
    .bufferImageHeight = 0,
    .imageSubresource  = VkImageSubresourceLayers {
      .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
      .mipLevel       = 0,
      .baseArrayLayer = 0,
      .layerCount     = 1,
    },
    .imageOffset = VkOffset3D {
      .x = 0,
      .y = 0,
      .z = 0,
    },
    .imageExtent = image->extent()
  };
  vkCmdCopyBufferToImage(commandBuffer, _buffer, image->image(), imageLayout, 1, &wholeRegion);
  VkFence fence = device.submitOneShotCommandBuffer(commandBuffer);
  vkWaitForFences(device.device, 1, &fence, VK_TRUE, UINT64_MAX);
  vkFreeCommandBuffers(device.device, device.commandPool, 1, &commandBuffer);
  vkDestroyFence(device.device, fence, nullptr);
}

void Buffer::copyTo(const Buffer* other) const {
  VkCommandBuffer commandBuffer = device.getOneShotCommandBuffer();
  const VkBufferCopy wholeRegion{
      .srcOffset = 0,
      .dstOffset = 0,
      .size      = allocationInfo.size
  };
  vkCmdCopyBuffer(commandBuffer, _buffer, other->_buffer, 1, &wholeRegion);
  VkFence fence = device.submitOneShotCommandBuffer(commandBuffer);
  vkWaitForFences(device.device, 1, &fence, VK_TRUE, UINT64_MAX);
  vkFreeCommandBuffers(device.device, device.commandPool, 1, &commandBuffer);
  vkDestroyFence(device.device, fence, nullptr);
}

VkBuffer Buffer::buffer() const {
  return _buffer;
}

void* Buffer::getObject() const {
  return _buffer;
}

void* Buffer::getView() const {
  return _view;
}
