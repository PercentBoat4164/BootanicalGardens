#include "DescriptorAllocator.hpp"

#include "GraphicsDevice.hpp"
#include "GraphicsInstance.hpp"

#include <volk.h>

DescriptorAllocator::DescriptorAllocator(const GraphicsDevice& device) : device(device) {
  std::vector<VkDescriptorPoolSize> sizes {
      {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 10}
  };
  const VkDescriptorPoolCreateInfo descriptorPoolCreateInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .maxSets = 10,
    .poolSizeCount = static_cast<uint32_t>(sizes.size()),
    .pPoolSizes = sizes.data()
  };
  GraphicsInstance::showError(vkCreateDescriptorPool(device.device, &descriptorPoolCreateInfo, nullptr, &pool), "Failed to create descriptor pool.");
}

DescriptorAllocator::~DescriptorAllocator() {
  vkDestroyDescriptorPool(device.device, pool, nullptr);
  pool = VK_NULL_HANDLE;
}

void DescriptorAllocator::clear() const {
  vkResetDescriptorPool(device.device, pool, 0);
}

std::vector<VkDescriptorSet> DescriptorAllocator::allocate(const std::vector<VkDescriptorSetLayout>& layouts) const {
  const VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .pNext = nullptr,
    .descriptorPool = pool,
    .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
    .pSetLayouts = layouts.data()
  };
  std::vector<VkDescriptorSet> sets(layouts.size());
  vkAllocateDescriptorSets(device.device, &descriptorSetAllocateInfo, sets.data());
  return sets;
};