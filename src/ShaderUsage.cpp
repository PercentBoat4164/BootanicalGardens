#include "ShaderUsage.hpp"

#include "Renderer.hpp"

#include <utility>

ShaderUsage::ShaderUsage(const GraphicsDevice& device, const Shader& shader, std::unordered_map<std::string, Resource*> resources, const Renderer& renderer) : shader(shader), resources(std::move(resources)), set(renderer.descriptorAllocator.allocate({shader.layout}).back()) {
  VkWriteDescriptorSet writeDescriptorSet {
    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .pNext = nullptr,
    .dstSet = set,
    .dstBinding = ,
    .dstArrayElement = 0,
    .descriptorCount = 1,
    .descriptorType = ,
    .pImageInfo = ,
  };
  vkUpdateDescriptorSets(device.device, 1, &writeDescriptorSet, 0, nullptr);
}