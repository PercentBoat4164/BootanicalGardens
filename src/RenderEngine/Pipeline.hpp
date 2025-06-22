#pragma once

#include "RenderGraph.hpp"


#include <memory>

#include <vulkan/vulkan_core.h>

class GraphicsDevice;
class Shader;

class Pipeline : public DescriptorSetRequirer {
  const std::shared_ptr<GraphicsDevice> device;
  VkPipeline pipeline{VK_NULL_HANDLE};
  VkPipelineLayout layout{VK_NULL_HANDLE};
  std::shared_ptr<Material> material;

public:
  VkPipelineBindPoint bindPoint;

  Pipeline(const std::shared_ptr<GraphicsDevice>& device, const std::shared_ptr<Material>& material);
  void bake(const std::shared_ptr<const RenderPass>& renderPass, std::span<VkDescriptorSetLayout> layouts);
  void update();
  ~Pipeline();

  [[nodiscard]] VkPipeline getPipeline() const;
  [[nodiscard]] VkPipelineLayout getLayout() const;
  [[nodiscard]] std::shared_ptr<Material> getMaterial() const;
};