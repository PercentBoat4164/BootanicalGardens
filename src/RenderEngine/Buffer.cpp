#include "Buffer.hpp"

#include <utility>
#include <volk.h>

#include "GraphicsDevice.hpp"
#include "GraphicsInstance.hpp"

Buffer::Buffer(const GraphicsDevice& device, std::string name) : device(device), _name(std::move(name)) {}

Buffer::Buffer(const GraphicsDevice& device, std::string name, const std::size_t size, const VkBufferUsageFlags usage) : Buffer(device, std::move(name)) {
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
      .usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
      .pUserData = this,
  };
  GraphicsInstance::showError(vmaCreateBuffer(device.allocator, &bufferCreateInfo, &allocationCreateInfo, &buffer, &allocation, &allocationInfo), "Failed to create buffer [" + _name + "].");
  vmaSetAllocationName(device.allocator, allocation, _name.c_str());
}

Buffer::Buffer(const GraphicsDevice& device, std::string name, const std::size_t size, const std::byte* data, const VkBufferUsageFlags usage) : Buffer(device, std::move(name), size, usage) {
  void* memory;
  GraphicsInstance::showError(vmaMapMemory(device.allocator, allocation, &memory), "Failed to map buffer [" + _name + "] memory.");
  std::memcpy(memory, data, size);
  vmaUnmapMemory(device.allocator, allocation);
}

Buffer::Buffer(const GraphicsDevice& device, std::string name, const std::vector<std::byte>& data, const VkBufferUsageFlags usage) : Buffer(device, std::move(name), data.size(), data.data(), usage) {}

Buffer::~Buffer() {
  vmaDestroyBuffer(device.allocator, buffer, allocation);
  buffer     = VK_NULL_HANDLE;
  allocation = VK_NULL_HANDLE;
}

void Buffer::copyToImage(const Image* image, const VkImageLayout imageLayout) const {
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
  vkCmdCopyBufferToImage(commandBuffer, buffer, image->image(), imageLayout, 1, &wholeRegion);
  VkFence fence = device.submitOneShotCommandBuffer(commandBuffer);
  vkWaitForFences(device.device, 1, &fence, VK_TRUE, UINT64_MAX);
  vkFreeCommandBuffers(device.device, device.commandPool, 1, &commandBuffer);
  vkDestroyFence(device.device, fence, nullptr);
}
