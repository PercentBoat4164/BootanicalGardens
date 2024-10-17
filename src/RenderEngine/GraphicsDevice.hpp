#pragma once
#include <VkBootstrap.h>
#include <vk_mem_alloc.h>

#include <memory>

class Texture;
class Mesh;
class Material;
class Shader;

class GraphicsDevice {
public:
  vkb::Device device;
  VkQueue globalQueue;
  uint32_t globalQueueFamilyIndex;
  VmaAllocator allocator{VK_NULL_HANDLE};
  VkCommandPool commandPool{VK_NULL_HANDLE};

  explicit GraphicsDevice();
  ~GraphicsDevice();

  [[nodiscard]] VkCommandBuffer getOneShotCommandBuffer() const;
  VkFence submitOneShotCommandBuffer(VkCommandBuffer commandBuffer) const;

  void destroy();
};
