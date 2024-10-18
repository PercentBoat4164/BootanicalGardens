#pragma once

#include <vk_mem_alloc.h>

#include <cstddef>
#include <string>
#include <vector>

class Image;
class GraphicsDevice;

class Buffer {
  const GraphicsDevice& device;
  VkBuffer buffer {VK_NULL_HANDLE};
  VmaAllocation allocation{VK_NULL_HANDLE};
  VmaAllocationInfo allocationInfo{};
  std::string _name;

public:
  explicit Buffer(const GraphicsDevice& device, std::string name);
  /**
  * @param size Size of buffer to create in bytes.
  */
  explicit Buffer(const GraphicsDevice& device, std::string name, std::size_t size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
  /**
  * @param size Number of elements in array at <c>data</c>, not number of bytes.
  */
  template<typename T> explicit Buffer(const GraphicsDevice& device, std::string name, std::size_t size, const T* data, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
  template<typename T> explicit Buffer(const GraphicsDevice& device, std::string name, const std::vector<T>& data, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
  ~Buffer();

  void copyTo(const Image* image, VkImageLayout imageLayout) const;
  void copyTo(const Buffer* other) const;
};

#include "GraphicsInstance.hpp"
#include "GraphicsDevice.hpp"

template <typename T> Buffer::Buffer(const GraphicsDevice& device, std::string name, const std::size_t size, const T* data, const VkBufferUsageFlags usage, const VmaMemoryUsage memoryUsage) : Buffer(device, std::move(name), size * sizeof(T), usage, memoryUsage) {
  void* memory;
  GraphicsInstance::showError(vmaMapMemory(device.allocator, allocation, &memory), "Failed to map buffer [" + _name + "] memory.");
  std::memcpy(memory, data, size * sizeof(T));
  vmaUnmapMemory(device.allocator, allocation);
}

template <typename T> Buffer::Buffer(const GraphicsDevice& device, std::string name, const std::vector<T>& data, const VkBufferUsageFlags usage, const VmaMemoryUsage memoryUsage) : Buffer(device, std::move(name), data.size(), data.data(), usage, memoryUsage) {}