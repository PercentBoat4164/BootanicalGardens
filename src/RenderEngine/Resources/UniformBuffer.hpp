#pragma once

#include "src/RenderEngine/GraphicsDevice.hpp"
#include "src/RenderEngine/Resources/Buffer.hpp"
#include "src/RenderEngine/Resources/StagingBuffer.hpp"

template<typename UniformData> class UniformBuffer : public Buffer {
  std::shared_ptr<StagingBuffer> stagingBuffer;

public:
  /**
   * Creates an empty Buffer suitable for uniform data.
   * @param device The GraphicsDevice that this UniformBuffer belongs to.
   * @param name The name of this UniformBuffer -- used for debugging purposes.
   */
  UniformBuffer(std::shared_ptr<GraphicsDevice> device, const char * const name) : Buffer(device, name, sizeof(UniformData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT) {
    VkMemoryPropertyFlags properties;
    vmaGetAllocationMemoryProperties(device->allocator, allocation, &properties);

    // Memory is not host visible, so a host-visible staging buffer must be used for uploads.
    if (properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) stagingBuffer = std::make_shared<StagingBuffer>(device, (std::string(name) + " -- StagingBuffer").c_str(), sizeof(UniformData));
  }

  /**
   * Creates a Buffer suitable for uniform data, then fills it with data. Equivalent to calling @link update @endlink after construction.
   * @param device The GraphicsDevice that this UniformBuffer belongs to.
   * @param name The name of this UniformBuffer -- used for debugging purposes.
   * @param data The data to fill the buffer with after it has been created.
   */
  UniformBuffer(std::shared_ptr<GraphicsDevice> device, const char * const name, const UniformData& data) : UniformBuffer(device, name) {
    update(data);
  }

  /**
   * Overwrites the data stored in the UniformBuffer with the @code newData@endcode.
   * @param newData The new data to fill the buffer with.
   */
  void update(const UniformData& newData) {
    const std::shared_ptr<BufferMapping> mapping = (stagingBuffer ? static_cast<Buffer*>(this) : static_cast<Buffer*>(stagingBuffer.get()))->map();
    std::memcpy(mapping->data, &newData, sizeof(UniformData));
  }
};