#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include <cstddef>
#include <string>
#include <vector>
#include "Image.hpp"

class GraphicsDevice;

class Buffer {
  const GraphicsDevice& device;
  VkBuffer buffer {VK_NULL_HANDLE};
  VmaAllocation allocation{VK_NULL_HANDLE};
  VmaAllocationInfo allocationInfo{};
  std::string _name;

public:
  explicit Buffer(const GraphicsDevice& device, std::string name);
  explicit Buffer(const GraphicsDevice& device, std::string name, std::size_t size, VkBufferUsageFlags usage);
  explicit Buffer(const GraphicsDevice& device, std::string name, std::size_t size, const std::byte* data, VkBufferUsageFlags usage);
  explicit Buffer(const GraphicsDevice& device, std::string name, const std::vector<std::byte>&data, VkBufferUsageFlags usage);
  ~Buffer();

  void copyToImage(const Image* image, VkImageLayout imageLayout) const;
};