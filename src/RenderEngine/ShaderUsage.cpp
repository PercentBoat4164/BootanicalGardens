#include "ShaderUsage.hpp"

#include "GraphicsInstance.hpp"
#include "Renderer.hpp"

#include <utility>
#include <ranges>

ShaderUsage::ShaderUsage(const GraphicsDevice& device, const Shader& shader, const std::unordered_map<std::string, Resource&>& resources, const Renderer& renderer) : shader(shader), resources(resources), set(renderer.descriptorAllocator.allocate({shader.layout}).back()) {
  std::vector<VkDescriptorImageInfo> imageInfos;
  imageInfos.reserve(resources.size());
  std::vector<VkWriteDescriptorSet> writeDescriptorSets;
  writeDescriptorSets.reserve(resources.size());
  for (const Resource& resource : std::ranges::views::values(resources)) {
    /**@todo Generalize this for multiple bindings of different types and so on.*/
    VkWriteDescriptorSet writeDescriptorSet {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pNext = nullptr,
      .dstSet = set,
      .dstBinding = 0,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
    };
    if (resource.type == Resource::Image) {
      const auto& image = dynamic_cast<const Image&>(resource);
      imageInfos.emplace_back(VK_NULL_HANDLE, image.view(), VK_IMAGE_LAYOUT_GENERAL);
      writeDescriptorSet.pImageInfo = &imageInfos.back();
    }
    writeDescriptorSets.push_back(writeDescriptorSet);
  }
  vkUpdateDescriptorSets(device.device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
}