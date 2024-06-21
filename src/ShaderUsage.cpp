#include "ShaderUsage.hpp"

#include "GraphicsInstance.hpp"
#include "Renderer.hpp"

#include <utility>

ShaderUsage::ShaderUsage(const GraphicsDevice& device, const Shader& shader, std::unordered_map<std::string, Resource*> resources, const Renderer& renderer) : shader(shader), resources(std::move(resources)), set(renderer.descriptorAllocator.allocate({shader.layout}).back()) {
  constexpr VkDescriptorImageInfo descriptorImageInfo {};
  const VkWriteDescriptorSet writeDescriptorSet {
    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .pNext = nullptr,
    .dstSet = set,
    .dstBinding = 0,
    .dstArrayElement = 0,
    .descriptorCount = 1,
    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
    .pImageInfo = &descriptorImageInfo,
  };
  vkUpdateDescriptorSets(device.device, 1, &writeDescriptorSet, 0, nullptr);
}