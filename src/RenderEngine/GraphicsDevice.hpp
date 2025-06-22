#pragma once

#include "src/RenderEngine/CommandBuffer.hpp"
#include "src/RenderEngine/DescriptorSetRequirements.hpp"

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
  [[nodiscard]] std::unique_ptr<VkDescriptorPool, std::function<void(VkDescriptorPool*)>> createDescriptorPool(const DescriptorSetRequirements& requirements, uint32_t copies, VkDescriptorPoolCreateFlags flags) const;
  [[nodiscard]] std::vector<std::vector<VkDescriptorSet>> allocateDescriptorSets(VkDescriptorPool pool, const std::vector<VkDescriptorSetLayout>& layouts, uint32_t copies) const;

  void destroy();
};
