#include "DescriptorSetAllocator.hpp"

#include "GraphicsDevice.hpp"
#include "GraphicsInstance.hpp"

#include <ranges>
#include <volk/volk.h>

std::vector<std::shared_ptr<VkDescriptorSet>> DescriptorSetAllocator::allocate(const std::vector<std::vector<VkDescriptorSetLayoutBinding>>& perSetBindings, const std::vector<VkDescriptorSetLayout>& layouts) {
  std::vector<std::vector<VkDescriptorSetLayout>> perPoolSetLayouts(pools.size() + 1);  // The sets that will be allocated from each pool
  std::vector<bool> perSetHasBeenConsumed(layouts.size());
  uint32_t i = std::numeric_limits<uint32_t>::max();
  for (const PoolData& pool: pools) {
    ++i;
    std::map<VkDescriptorType, uint32_t> sizes;  // The sizes that will be allocated from this pool
    for (uint32_t j{}; j < perSetBindings.size(); ++j) {
      if (perSetHasBeenConsumed[j]) continue;
      bool allBindingsFit = true;  // Does this descriptor set fit into this pool?
      for (const VkDescriptorSetLayoutBinding& binding: perSetBindings[j]) {
        auto descriptorInPool = pool.availableDescriptors.find(binding.descriptorType);
        if (descriptorInPool == pool.availableDescriptors.end()) { allBindingsFit = false; break; }  // Break early if this descriptor type is not available in the pool.
        // descriptorToBeAdded represents the descriptors of this type already placed in this pool by earlier sets in this same allocation. They have not yet been added to the pool's sizes.
        if (const auto descriptorToBeAdded = sizes.find(binding.descriptorType); descriptorInPool->second < binding.descriptorCount + (descriptorToBeAdded == sizes.end() ? 0 : descriptorToBeAdded->second)) { allBindingsFit = false; break; }
      }
      if (!allBindingsFit) continue;  // If this descriptor set does not fit, keep looking
      // Otherwise, push it into the list of descriptors to put into this
      for (const VkDescriptorSetLayoutBinding& binding: perSetBindings[j]) sizes.emplace(binding.descriptorType, binding.descriptorCount);
      perPoolSetLayouts[i].push_back(layouts[j]);
      perSetHasBeenConsumed[j] = true;
    }
  }
  // Create a new descriptor pool to contain any descriptor sets that cannot fit into any other pool
  std::map<VkDescriptorType, uint32_t> sizes;  // Required pool sizes
  uint32_t setCount{};
  for (uint32_t j{}; j < perSetBindings.size(); ++j) if (!perSetHasBeenConsumed[j]) {
    ++setCount;
    for (const VkDescriptorSetLayoutBinding& binding: perSetBindings[j]) sizes[binding.descriptorType] += binding.descriptorCount;
  }
  if (setCount > 0) {
    std::vector<VkDescriptorPoolSize> poolSizes;
    for (const auto& [type, count]: sizes) poolSizes.emplace_back(type, count);
    const VkDescriptorPoolCreateInfo createInfo {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext         = nullptr,
        .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets       = setCount,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes    = poolSizes.data(),
    };
    if (const VkResult result = vkCreateDescriptorPool(device.device, &createInfo, nullptr, &pools.emplace(VK_NULL_HANDLE, sizes)->pool); result != VK_SUCCESS) GraphicsInstance::showError("failed to create descriptor pool");
    for (uint32_t j{}; j < perSetBindings.size(); ++j) if (!perSetHasBeenConsumed[j]) perPoolSetLayouts.back().push_back(layouts[j]);
  }

  // Allocate descriptor sets
  VkDescriptorSetAllocateInfo allocInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .pNext = nullptr,
  };
  std::vector<VkDescriptorSet> descriptorSets(layouts.size());
  std::vector<std::shared_ptr<VkDescriptorSet>> sets;
  sets.reserve(layouts.size());
  uint32_t offset = 0;
  i = std::numeric_limits<uint32_t>::max();
  for (PoolData& pool : pools) {
    allocInfo.descriptorPool = pool.pool;
    allocInfo.descriptorSetCount = perPoolSetLayouts[++i].size();
    allocInfo.pSetLayouts = perPoolSetLayouts[i].data();
    if (const VkResult result = vkAllocateDescriptorSets(device.device, &allocInfo, &descriptorSets[offset]); result != VK_SUCCESS) GraphicsInstance::showError("failed to allocate descriptor sets");
    const std::vector<VkDescriptorSetLayoutBinding>& poolSizes = perSetBindings[i];
    for (uint32_t j{offset}; j < offset + allocInfo.descriptorSetCount; ++j) {
      sets.push_back(std::shared_ptr<VkDescriptorSet>(new VkDescriptorSet{descriptorSets[j]}, [this, &pool, poolSizes](const VkDescriptorSet* set) {
          /**@todo: Put descriptor sets into a bucket to be batch freed later (potentially at the next descriptor set allocation).*/
          vkFreeDescriptorSets(device.device, pool.pool, 1, set);
          for (const VkDescriptorSetLayoutBinding& poolSize: poolSizes) pool.availableDescriptors.at(poolSize.descriptorType) += poolSize.descriptorCount;
          delete set;
      }));
    }
    offset += allocInfo.descriptorSetCount;
  }

  return sets;
}

void DescriptorSetAllocator::destroy() {
  for (const PoolData& pool: pools) vkDestroyDescriptorPool(device.device, pool.pool, nullptr);
}