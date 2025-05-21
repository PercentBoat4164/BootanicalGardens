#pragma once

#include "CommandBuffer.hpp"
#include "DescriptorAllocator.hpp"

#include <VkBootstrap.h>
#include <vma/vk_mem_alloc.h>

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

  /// Used to allocate the per-frame descriptor sets.
  ///
  /// FRAMES_IN_FLIGHT descriptor sets
  DescriptorAllocator perFrameDescriptorAllocator;

  /// Used to allocate the per-pass descriptor sets.
  ///
  /// FRAMES_IN_FLIGHT * passCount descriptor sets
  DescriptorAllocator perPassDescriptorAllocator;

  /// Used to allocate the per-material descriptor sets.
  ///
  /// FRAMES_IN_FLIGHT * materialCount descriptor sets
  DescriptorAllocator perMaterialDescriptorAllocator;

  /// Used to allocate the per-object descriptor sets.
  ///
  /// FRAMES_IN_FLIGHT * objectCount descriptor sets
  DescriptorAllocator perObjectDescriptorAllocator;

  explicit GraphicsDevice();
  ~GraphicsDevice();

  struct ImmediateExecutionContext {
    VkCommandBuffer commandBuffer;
    VkFence fence;
  };

  [[nodiscard]] ImmediateExecutionContext executeCommandBufferImmediateAsync(const CommandBuffer& commandBuffer) const;
  void waitForAsyncCommandBuffer(ImmediateExecutionContext context) const;
  void executeCommandBufferImmediate(const CommandBuffer& commandBuffer) const;

  void destroy();
};
