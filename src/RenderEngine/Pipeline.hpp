#pragma once

#include "RenderGraph.hpp"


#include <memory>
#include <vector>

#include <vulkan/vulkan_core.h>

class GraphicsDevice;
class Shader;

class Pipeline {
  const std::shared_ptr<GraphicsDevice> device;
  VkPipeline pipeline{VK_NULL_HANDLE};
  VkPipelineLayout layout{VK_NULL_HANDLE};
  std::vector<std::shared_ptr<VkDescriptorSet>> descriptorSets;
  std::shared_ptr<const Material> material;

public:
  VkPipelineBindPoint bindPoint;

  Pipeline(const std::shared_ptr<GraphicsDevice>& device, const std::shared_ptr<const Material>& material, const std::shared_ptr<const RenderPass>& renderPass);
  ~Pipeline();

  [[nodiscard]] VkPipeline getPipeline() const;
  [[nodiscard]] VkPipelineLayout getLayout() const;
  [[nodiscard]] std::shared_ptr<VkDescriptorSet> getDescriptorSet(uint64_t frameIndex) const;
  void update();
};