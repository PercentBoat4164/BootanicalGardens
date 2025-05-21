#pragma once

#include "RenderGraph.hpp"


#include <memory>
#include <vector>

#include <vulkan/vulkan_core.h>

class GraphicsDevice;
class Shader;

class Pipeline {
  const std::shared_ptr<GraphicsDevice> device;
  std::vector<std::shared_ptr<Shader>> shaders;
  VkPipeline pipeline{VK_NULL_HANDLE};
  VkPipelineLayout layout{VK_NULL_HANDLE};
  std::vector<std::shared_ptr<VkDescriptorSet>> descriptorSets;

public:
  VkPipelineBindPoint bindPoint;

  Pipeline(const std::shared_ptr<GraphicsDevice>& device, std::shared_ptr<const Material> material, std::shared_ptr<const RenderPass> renderPass, VkPipelineLayout layout);
  ~Pipeline();

  [[nodiscard]] VkPipeline getPipeline() const;
  [[nodiscard]] VkPipelineLayout getLayout() const;
  [[nodiscard]] std::shared_ptr<VkDescriptorSet> getDescriptorSet(uint64_t frameIndex) const;
};