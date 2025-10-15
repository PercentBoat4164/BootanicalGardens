#pragma once
#include "RenderGraph.hpp"

#include <vulkan/vulkan_core.h>


#include <memory>
#include <ranges>
#include <vector>

class GraphicsDevice;
class RenderGraph;

class DescriptorSetRequirer {
  GraphicsDevice* const device;

protected:
  std::vector<std::shared_ptr<VkDescriptorSet>> descriptorSets;
  VkDescriptorSetLayout layout{VK_NULL_HANDLE};

public:
  explicit DescriptorSetRequirer(GraphicsDevice* device);
  virtual ~DescriptorSetRequirer();

  void setDescriptorSets(std::ranges::range auto&& sets, VkDescriptorSetLayout setLayout) {
    descriptorSets = std::ranges::to<std::vector>(sets);
    layout = setLayout;
  }

  /**
   * This function will be called after <c>setDescriptorSets</c> and when the descriptor sets need to be (re)written. This function's purpose is to enable the batching of the descriptor set writes. Rather than issuing your own <c>vkWriteDescriptorSets</c> calls, store the <c>VkDescriptorWrites</c> objects in <c>writes</c>.
   * @param miscMemoryPool A pool of untyped memory that will be freed after writing all descriptor sets. Use this to store data pointed to by your <c>writes</c>.
   * @param writes A vector of all the descriptor writes to apply. These will all be applied in one shot. Any data stored in the pointers of the writes must be valid after this function exits. It is recommended to use the <c>miscMemoryPool</c> for this purpose.
   * @param graph
   */
  virtual void writeDescriptorSets(std::vector<void*>& miscMemoryPool, std::vector<VkWriteDescriptorSet>& writes, const RenderGraph& graph) = 0;
  [[nodiscard]] std::shared_ptr<VkDescriptorSet> getDescriptorSet(std::size_t index) const;
};