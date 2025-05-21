#pragma once

#include <plf_colony.h>
#include <vulkan/vulkan.h>

#include <vector>

class GraphicsDevice;

class DescriptorAllocator {
  struct PoolInfo {
    VkDescriptorPool pool;
    uint32_t availableSets;
    uint32_t totalSets;
  };
  const GraphicsDevice& device;
  VkDescriptorSetLayout layout;
  plf::colony<PoolInfo> pools{};
  plf::colony<VkDescriptorSet> sets{};
  std::vector<VkDescriptorPoolSize> sizes;

public:
  explicit DescriptorAllocator(const GraphicsDevice& device);
  ~DescriptorAllocator();

  void prepareAllocation(const std::vector<VkDescriptorSetLayoutBinding>& bindings);
  void clear();
  std::vector<std::shared_ptr<VkDescriptorSet>> allocate(uint32_t descriptorCount);
  /// Merges all descriptor sets into a single pool.
  void optimize();
  void destroy();
  
  VkDescriptorSetLayout getLayout() const { return layout; }
};