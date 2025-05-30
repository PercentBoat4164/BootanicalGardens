#pragma once

#include "Buffer.hpp"

#include "src/RenderEngine/GraphicsDevice.hpp"


class StagingBuffer : public Buffer {
public:
  /**
   * Creates an empty host visible buffer. This buffer can then be filled with data and copied into a device visible buffer.
   * @param device The <c>GraphicsDevice</c> that this buffer is associated with.
   * @param name The name of the buffer - to be used for debugging purposes.
   * @param size The size of the buffer in bytes.
   */
  explicit StagingBuffer(const std::shared_ptr<GraphicsDevice>& device, const char* name, VkDeviceSize size);

  /**
   * Creates a host visible buffer and fills it with data. This buffer can then be copied into a device visible buffer.
   * @param device The <c>GraphicsDevice</c> that this buffer is associated with.
   * @param name The name of the buffer - to be used for debugging purposes.
   * @param data Pointer to an array of <c>size</c> <c>T</c> elements.
   * @param count Number of elements in the array at <c>data</c>, not number of bytes.
   */
  template<typename T> explicit StagingBuffer(const std::shared_ptr<GraphicsDevice> device, const char* name, const T* data, const VkDeviceSize count) : StagingBuffer(device, name, sizeof(T) * count) {
    vmaCopyMemoryToAllocation(device->allocator, data, allocation, 0, allocationInfo.size);
  }

  /**
   * Creates a host visible buffer and fills it with data. This buffer can then be copied into a device visible buffer.
   * @param device The <c>GraphicsDevice</c> that this buffer is associated with.
   * @param name The name of the buffer - to be used for debugging purposes.
   * @param data Pointer to an array of <c>size</c> <c>T</c> elements.
   */
  template<typename T> explicit StagingBuffer(const std::shared_ptr<GraphicsDevice> device, const char* name, const std::vector<T>& data) : StagingBuffer(device, name, data.data(), data.size()) {}
};