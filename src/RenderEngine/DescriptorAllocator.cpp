#include "DescriptorAllocator.hpp"

#include "GraphicsDevice.hpp"
#include "GraphicsInstance.hpp"

#include <volk/volk.h>

DescriptorAllocator::DescriptorAllocator(const GraphicsDevice& device) : device(device) {}

DescriptorAllocator::~DescriptorAllocator() {
  destroy();
}

void DescriptorAllocator::prepareAllocation(const std::vector<VkDescriptorSetLayoutBinding>& bindings) {
  for (const VkDescriptorSetLayoutBinding& binding : bindings) {
    bool typeFound = false;
    for (VkDescriptorPoolSize& size : sizes) {
      if (size.type == binding.descriptorType) {
        size.descriptorCount += binding.descriptorCount;
        typeFound = true;
        break;
      }
    }
    if (!typeFound) sizes.emplace_back(binding.descriptorType, binding.descriptorCount);
  }

  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .bindingCount = static_cast<uint32_t>(bindings.size()),
    .pBindings = bindings.data()
};
  vkCreateDescriptorSetLayout(device.device, &descriptorSetLayoutCreateInfo, nullptr, &layout);
}

void DescriptorAllocator::clear() {
  for (PoolInfo& pool : pools) {
    vkResetDescriptorPool(device.device, pool.pool, 0);
    pool.availableSets = pool.totalSets;
  }
  pools.clear();
}

std::vector<std::shared_ptr<VkDescriptorSet>> DescriptorAllocator::allocate(const uint32_t descriptorCount) {
  std::vector<VkDescriptorSet> descriptorSets(descriptorCount);
  std::vector layouts(descriptorCount, layout);
  bool allocated = false;
  VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{
      .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .pNext              = nullptr,
      .descriptorPool     = VK_NULL_HANDLE,
      .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
      .pSetLayouts        = layouts.data()
  };
  for (PoolInfo& pool: pools) {
    descriptorSetAllocateInfo.descriptorPool = pool.pool;
    if (descriptorCount <= pool.availableSets) {
      allocated = vkAllocateDescriptorSets(device.device, &descriptorSetAllocateInfo, descriptorSets.data()) == VK_SUCCESS;
      pool.availableSets -= descriptorCount;
      break;
    }
  }
  if (!allocated) {
    std::vector scaledSizes = sizes;
    for (VkDescriptorPoolSize& size: scaledSizes) size.descriptorCount *= descriptorCount;
    
    const VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext         = nullptr,
        .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets       = descriptorCount,
        .poolSizeCount = static_cast<uint32_t>(scaledSizes.size()),
        .pPoolSizes    = scaledSizes.data()
    };
    PoolInfo& pool = *pools.emplace();
    pool.totalSets = descriptorCount;
    pool.availableSets = 0;
    if (const VkResult result = vkCreateDescriptorPool(device.device, &descriptorPoolCreateInfo, nullptr, &pool.pool); result != VK_SUCCESS) GraphicsInstance::showError(result, "Failed to create descriptor pool.");
    descriptorSetAllocateInfo.descriptorPool = pool.pool;
    if (const VkResult result = vkAllocateDescriptorSets(device.device, &descriptorSetAllocateInfo, descriptorSets.data()); result != VK_SUCCESS) GraphicsInstance::showError(result, "Failed to allocate descriptor descriptorSets.");
  }
  layouts.clear();
  std::vector<std::shared_ptr<VkDescriptorSet>> result(descriptorCount);
  VkDescriptorPool pool = descriptorSetAllocateInfo.descriptorPool;
  VkDevice dev = device.device;
  for (uint32_t i{}; i < descriptorCount; ++i)
    result[i] = std::shared_ptr<VkDescriptorSet>(new VkDescriptorSet{descriptorSets[i]}, [dev, pool](VkDescriptorSet *set){ vkFreeDescriptorSets(dev, pool, 1, set); });
  return result;
}

void DescriptorAllocator::optimize() {
}

void DescriptorAllocator::destroy() {
  for (const PoolInfo& pool: pools) vkDestroyDescriptorPool(device.device, pool.pool, nullptr);
  pools.clear();
  if (layout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device.device, layout, nullptr);
  layout = VK_NULL_HANDLE;
}