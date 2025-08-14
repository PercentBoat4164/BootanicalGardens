#pragma once
#include <vulkan/vulkan_core.h>


#include <memory>
#include <span>
#include <vector>

class GraphicsDevice;

class DescriptorSetRequirer {
  GraphicsDevice* const device;

protected:
  std::vector<std::shared_ptr<VkDescriptorSet>> descriptorSets;
  VkDescriptorSetLayout layout{VK_NULL_HANDLE};

public:
  explicit DescriptorSetRequirer(GraphicsDevice* device);
  virtual ~DescriptorSetRequirer();

  void setDescriptorSets(std::span<std::shared_ptr<VkDescriptorSet>> sets, VkDescriptorSetLayout setLayout);
  virtual void writeDescriptorSets(std::vector<void*>& miscMemoryPool, std::vector<VkWriteDescriptorSet>& writes) = 0;
  [[nodiscard]] std::shared_ptr<VkDescriptorSet> getDescriptorSet(std::size_t index) const;
};