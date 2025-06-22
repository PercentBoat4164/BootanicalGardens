#pragma once

#include <vulkan/vulkan.h>

#include <map>
#include <memory>
#include <set>
#include <vector>


class Pipeline;
class GraphicsDevice;

class DescriptorSetRequirer {
protected:
  std::vector<VkDescriptorSet> descriptorSets;

public:
  void setDescriptorSets(std::vector<VkDescriptorSet> sets) { descriptorSets = std::move(sets); }
  [[nodiscard]] VkDescriptorSet getDescriptorSet(const uint32_t frameIndex) const { return descriptorSets[frameIndex]; }
};

struct DescriptorSetRequirements {
  struct DescriptorSetRequirerDataIndex {
    std::size_t index;       // Index into the per-set vectors of this object's set.
    std::size_t startIndex;  // Index into the per-set vectors of this pipeline's first set.
    uint32_t setCount;       // Number of sets in this pipeline.
  };
  std::map<VkDescriptorType, uint32_t> sizes;
  std::map<std::shared_ptr<DescriptorSetRequirer>, DescriptorSetRequirerDataIndex> objectDataIndex;

  // Each of the vectors below contains data for all pipelines. Get a start index for a specific pipeline using <c>objectDataIndex[pipeline].index;</c>
  std::vector<std::vector<uint64_t>> bindingIDs;                       // ID of each binding in the order that they are bound in the set. Indicates what to bind where.
  std::vector<std::vector<VkDescriptorSetLayoutBinding>> setBindings;  // Layout of the set.

  DescriptorSetRequirements operator+(const DescriptorSetRequirements& other) const;
  DescriptorSetRequirements& operator+=(const DescriptorSetRequirements& other);
};