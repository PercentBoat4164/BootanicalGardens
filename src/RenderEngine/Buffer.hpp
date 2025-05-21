#pragma once

#include "Resource.hpp"

#include <vma/vk_mem_alloc.h>

class Image;
class GraphicsDevice;

class Buffer : public Resource {
protected:
  VkBuffer buffer{VK_NULL_HANDLE};
  VkBufferView view{VK_NULL_HANDLE};
  VmaAllocation allocation{VK_NULL_HANDLE};
  VmaAllocationInfo allocationInfo{};

public:
  /**
   * @param device
   * @param name
   * @param bufferSize Size of buffer to create in bytes.
   * @param usage
   * @param required
   * @param preferred
   * @param memoryUsage
   * @param flags
   */
  explicit Buffer(const std::shared_ptr<GraphicsDevice>& device, const char* name, VkDeviceSize bufferSize, VkBufferUsageFlags usage, VkMemoryPropertyFlags required, VkMemoryPropertyFlags preferred, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags flags);
  ~Buffer() override;

  [[nodiscard]] VkBuffer getBuffer() const;
  [[nodiscard]] VkDeviceSize getSize() const;

private:
  [[nodiscard]] void* getObject() const override;
  [[nodiscard]] void* getView() const override;
};