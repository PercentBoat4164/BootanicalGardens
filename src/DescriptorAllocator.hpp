#pragma once

#include <vulkan/vulkan.h>

#include <vector>

class GraphicsDevice;

class DescriptorAllocator {
  const GraphicsDevice& device;
  VkDescriptorPool pool{VK_NULL_HANDLE};

public:
  explicit DescriptorAllocator(const GraphicsDevice& device);
  ~DescriptorAllocator();

  void clear() const;
  [[nodiscard]] std::vector<VkDescriptorSet> allocate(const std::vector<VkDescriptorSetLayout>& layouts) const;
};