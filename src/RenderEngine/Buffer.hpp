#pragma once

#include "Resource.hpp"

#include <vk_mem_alloc.h>

#include <cstddef>
#include <string>
#include <vector>

class Image;
class GraphicsDevice;

class Buffer : public Resource {
  VkBuffer _buffer {VK_NULL_HANDLE};
  VkBufferView _view {VK_NULL_HANDLE};
  VmaAllocation allocation{VK_NULL_HANDLE};
  VmaAllocationInfo allocationInfo{};

public:
  /**
  * @param size Size of buffer to create in bytes.
  */
  explicit Buffer(const GraphicsDevice& device, const std::string& name, std::size_t size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
  /**
  * @param size Number of elements in array at <c>data</c>, not number of bytes.
  */
  template<typename T> explicit Buffer(const GraphicsDevice& device, std::string name, std::size_t size, const T* data, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
  template<typename T> explicit Buffer(const GraphicsDevice& device, std::string name, const std::vector<T>& data, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
  ~Buffer() override;

  [[nodiscard]] VkBuffer buffer() const;
  [[nodiscard]] uint64_t size() const;

private:
  [[nodiscard]] void* getObject() const override;
  [[nodiscard]] void* getView() const override;
};

#include "GraphicsInstance.hpp"
#include "GraphicsDevice.hpp"

template <typename T> Buffer::Buffer(const GraphicsDevice& device, std::string name, const std::size_t size, const T* data, const VkBufferUsageFlags usage, const VmaMemoryUsage memoryUsage) : Buffer(device, std::move(name), size * sizeof(T), usage, memoryUsage) {
  void* memory;
  GraphicsInstance::showError(vmaMapMemory(device.allocator, allocation, &memory), "Failed to map buffer [" + name + "] memory.");
  std::memcpy(memory, data, size * sizeof(T));
  vmaUnmapMemory(device.allocator, allocation);
}

template <typename T> Buffer::Buffer(const GraphicsDevice& device, std::string name, const std::vector<T>& data, const VkBufferUsageFlags usage, const VmaMemoryUsage memoryUsage) : Buffer(device, std::move(name), data.size(), data.data(), usage, memoryUsage) {}