#pragma once

#include "src/RenderEngine/Resources/Resource.hpp"

#include <vma/vk_mem_alloc.h>

class Image;
class GraphicsDevice;

class Buffer : public Resource {
public:
  struct BufferMapping;

private:
  std::weak_ptr<BufferMapping> mapping;

protected:
  VkBuffer buffer{VK_NULL_HANDLE};
  VkBufferView view{VK_NULL_HANDLE};
  VmaAllocation allocation{VK_NULL_HANDLE};
  VmaAllocationInfo allocationInfo{};

public:
  struct BufferMapping {
    BufferMapping(const std::shared_ptr<GraphicsDevice>& device, const std::shared_ptr<const Buffer>& buffer);
    ~BufferMapping();

    std::shared_ptr<GraphicsDevice> device;
    std::shared_ptr<const Buffer> buffer;
    void* data{};
  };

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
  [[nodiscard]] std::shared_ptr<BufferMapping> map();

private:
  [[nodiscard]] void* getObject() const override;
  [[nodiscard]] void* getView() const override;
};