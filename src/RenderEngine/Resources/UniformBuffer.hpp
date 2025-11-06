#pragma once

#include "src/RenderEngine/GraphicsDevice.hpp"
#include "src/RenderEngine/Resources/Buffer.hpp"
#include "src/RenderEngine/Resources/StagingBuffer.hpp"

template<typename UniformData> class UniformBuffer : public Buffer {
public:
  /**
   * Creates an empty Buffer suitable for uniform data.
   * @param device The GraphicsDevice that this UniformBuffer belongs to.
   * @param name The name of this UniformBuffer -- used for debugging purposes.
   */
  UniformBuffer(GraphicsDevice* device, const char * const name) : Buffer(device, name, sizeof(UniformData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT) {}

  /**
   * Creates a Buffer suitable for uniform data, then fills it with data. Equivalent to calling <code>update</code> after construction.
   * @param device The GraphicsDevice that this UniformBuffer belongs to.
   * @param name The name of this UniformBuffer -- used for debugging purposes.
   * @param data The data to fill the buffer with after it has been created.
   * @see UniformBuffer::update(const UniformData&)
   */
  UniformBuffer(GraphicsDevice* device, const char * const name, const UniformData& data) : UniformBuffer(device, name) {
    update(data);
  }

  /**
   * Overwrites the data stored in the UniformBuffer with the <code>newData</code>.
   * @param newData The new data to fill the buffer with.
   */
  void update(const UniformData& newData) {
    vmaCopyMemoryToAllocation(device->allocator, &newData, allocation, 0, sizeof(UniformData));
  }
};