#pragma once

#include "src/RenderEngine/DescriptorSetRequirer.hpp"
#include "src/RenderEngine/RenderGraph.hpp"

#include <vulkan/vulkan_core.h>

#include <memory>

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
  void bake(const std::shared_ptr<const RenderPass>& renderPass, std::span<VkDescriptorSetLayout> layouts, std::vector<void*>& miscMemoryPool, std::vector<VkGraphicsPipelineCreateInfo>& createInfos, std::vector<VkPipeline*>& pipelines);
  void writeDescriptorSets(std::vector<void*>& miscMemoryPool, std::vector<VkWriteDescriptorSet>& writes) override;
  void update();
  ~Pipeline() override;

  [[nodiscard]] VkPipeline getPipeline() const;
  [[nodiscard]] VkPipelineLayout getLayout() const;
  [[nodiscard]] std::shared_ptr<Material> getMaterial() const;
};