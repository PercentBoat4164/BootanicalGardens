#pragma once
#include <VkBootstrap.h>
#include <vk_mem_alloc.h>

class GraphicsDevice {
public:
  vkb::Device device;
  VkQueue queue;
  uint32_t queueFamilyIndex;
  VmaAllocator allocator{VK_NULL_HANDLE};

  explicit GraphicsDevice();
  ~GraphicsDevice();

  void destroy();
};
