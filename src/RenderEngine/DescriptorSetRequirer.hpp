#pragma once
#include <vulkan/vulkan_core.h>


#include <memory>
#include <vector>

class GraphicsDevice;

class DescriptorSetRequirer {
  const std::shared_ptr<GraphicsDevice> device;

protected:
  std::vector<std::shared_ptr<VkDescriptorSet>> descriptorSets;
  VkDescriptorSetLayout layout{VK_NULL_HANDLE};

public:
  explicit DescriptorSetRequirer(const std::shared_ptr<GraphicsDevice>& device);
  virtual ~DescriptorSetRequirer();

  void setDescriptorSets(std::span<std::shared_ptr<VkDescriptorSet>> sets, VkDescriptorSetLayout setLayout);
  virtual void writeDescriptorSets(std::vector<void*>& miscMemoryPool, std::vector<VkWriteDescriptorSet>& writes) = 0;
  [[nodiscard]] std::shared_ptr<VkDescriptorSet> getDescriptorSet(std::size_t index) const;
};