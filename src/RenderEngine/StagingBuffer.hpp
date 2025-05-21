#pragma once

#include "src/RenderEngine/Buffer.hpp"

#include "src/RenderEngine/GraphicsDevice.hpp"


class StagingBuffer : public Buffer {
public:
  /**
   * Creates a host visible buffer and fills it with data. This buffer can then be copied into a device visible buffer.
   * @param device The <c>GraphicsDevice</c> that this buffer is associated with.
   * @param name The name of the buffer - to be used for debugging purposes.
   * @param data Pointer to an array of <c>size</c> <c>T</c> elements.
   * @param count Number of elements in the array at <c>data</c>, not number of bytes.
   */
  template<typename T> explicit StagingBuffer(const std::shared_ptr<GraphicsDevice>& device, const char* name, const T* data, const VkDeviceSize count) :
      Buffer(device, name, sizeof(T) * count, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST, VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_STRATEGY_MIN_TIME_BIT) {
    vmaCopyMemoryToAllocation(device->allocator, data, allocation, 0, allocationInfo.size);
  }
  /**
   * Creates a host visible buffer and fills it with data. This buffer can then be copied into a device visible buffer.
   * @param device The <c>GraphicsDevice</c> that this buffer is associated with.
   * @param name The name of the buffer - to be used for debugging purposes.
   * @param data Pointer to an array of <c>size</c> <c>T</c> elements.
   */
  template<typename T> explicit StagingBuffer(const std::shared_ptr<GraphicsDevice>& device, const char* name, const std::vector<T>& data) : StagingBuffer(device, name, data.data(), data.size()) {}
};