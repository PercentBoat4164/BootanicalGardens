#include "DescriptorSetRequirer.hpp"

#include "src/RenderEngine/GraphicsDevice.hpp"

#include <volk/volk.h>

DescriptorSetRequirer::DescriptorSetRequirer(GraphicsDevice* const device) : device(device) {}

DescriptorSetRequirer::~DescriptorSetRequirer() {
  vkDestroyDescriptorSetLayout(device->device, layout, nullptr);
}

std::shared_ptr<VkDescriptorSet> DescriptorSetRequirer::getDescriptorSet(const std::size_t index) const {
  return descriptorSets[index];
}
