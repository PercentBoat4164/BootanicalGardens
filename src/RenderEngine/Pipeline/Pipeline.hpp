#pragma once

#include "src/RenderEngine/DescriptorSetRequirer.hpp"
#include "src/RenderEngine/RenderGraph.hpp"

#include <vulkan/vulkan_core.h>
#include <glm/matrix.hpp>

#include <deque>
#include <memory>

class GraphicsDevice;
class Shader;

class Pipeline : public DescriptorSetRequirer {
  GraphicsDevice* const device;
  VkPipeline pipeline{VK_NULL_HANDLE};
  VkPipelineLayout layout{VK_NULL_HANDLE};
  Material* material;
  std::shared_ptr<RenderPass> renderPass{nullptr};

public:
  VkPipelineBindPoint bindPoint;

  Pipeline(GraphicsDevice* device, Material* material);
  void bake(const std::shared_ptr<RenderPass>&renderPass, uint32_t subpassIndex, std::span<VkDescriptorSetLayout> layouts, std::deque<std::tuple<void*, std::function<void(void*)>>>& miscMemoryPool, std::vector<VkGraphicsPipelineCreateInfo>&createInfos, std::vector<VkPipeline*>& pipelines);
  void writeDescriptorSets(std::deque<std::tuple<void*, std::function<void(void*)>>>& miscMemoryPool, std::vector<VkWriteDescriptorSet>& writes) override;
  ~Pipeline() override;

  [[nodiscard]] VkPipeline getPipeline() const;
  [[nodiscard]] VkPipelineLayout getLayout() const;
  [[nodiscard]] Material* getMaterial() const;
};