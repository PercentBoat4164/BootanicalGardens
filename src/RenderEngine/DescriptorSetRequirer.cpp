#include "DescriptorSetRequirer.hpp"

#include "src/RenderEngine/GraphicsDevice.hpp"

#include <volk/volk.h>

DescriptorSetRequirer::DescriptorSetRequirer(GraphicsDevice* const device) : device(device) {}

DescriptorSetRequirer::~DescriptorSetRequirer() {
  vkDestroyDescriptorSetLayout(device->device, layout, nullptr);
}

void DescriptorSetRequirer::setDescriptorSets(std::span<std::shared_ptr<VkDescriptorSet>> sets, VkDescriptorSetLayout setLayout) {
  descriptorSets = {sets.begin(), sets.end()};
  layout = setLayout;
}

std::shared_ptr<VkDescriptorSet> DescriptorSetRequirer::getDescriptorSet(const std::size_t index) const {
  return descriptorSets[index];
}
