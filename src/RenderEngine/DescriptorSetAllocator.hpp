#pragma once
#include <vulkan/vulkan_core.h>

#include <plf_colony.h>

#include <map>
#include <vector>


class GraphicsDevice;
class DescriptorSetAllocator {
  struct PoolData {
    VkDescriptorPool pool;
    std::map<VkDescriptorType, uint32_t> availableDescriptors;
  };

  const GraphicsDevice& device;
  plf::colony<PoolData> pools;

public:
  explicit DescriptorSetAllocator(const GraphicsDevice& device) : device(device) {}
  std::vector<std::shared_ptr<VkDescriptorSet>> allocate(const std::vector<std::vector<VkDescriptorSetLayoutBinding>>& perSetBindings, const std::vector<VkDescriptorSetLayout>& layouts);
  void destroy();
};