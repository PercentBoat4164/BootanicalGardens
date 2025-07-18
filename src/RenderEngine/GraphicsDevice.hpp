#pragma once

#include "src/RenderEngine/DescriptorSetAllocator.hpp"
#include "src/RenderEngine/CommandBuffer.hpp"

#include <VkBootstrap.h>
#include <functional>
#include <vma/vk_mem_alloc.h>

#include <map>
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
  std::map<std::size_t, std::weak_ptr<VkSampler>> samplers;
  DescriptorSetAllocator descriptorSetAllocator{*this};

  explicit GraphicsDevice();
  ~GraphicsDevice();

  struct ImmediateExecutionContext {
    VkCommandBuffer commandBuffer;
    VkFence fence;
  };

  [[nodiscard]] ImmediateExecutionContext executeCommandBufferAsync(const CommandBuffer& commandBuffer) const;
  void waitForAsyncCommandBuffer(ImmediateExecutionContext context) const;
  void executeCommandBufferImmediate(const CommandBuffer& commandBuffer) const;
  std::shared_ptr<VkSampler> getSampler(VkFilter magnificationFilter = VK_FILTER_NEAREST, VkFilter minificationFilter = VK_FILTER_NEAREST, VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST, VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, float lodBias = 0, VkBorderColor borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK);

  void destroy();
};
