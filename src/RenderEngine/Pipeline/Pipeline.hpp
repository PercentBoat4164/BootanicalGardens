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
  struct LightData {
    glm::mat4 light_ViewProjectionMatrix;
    glm::vec3 light_Position;
  };
  GraphicsDevice* const device;
  VkPipeline pipeline{VK_NULL_HANDLE};
  VkPipelineLayout layout{VK_NULL_HANDLE};
  Material* material;
  std::unique_ptr<UniformBuffer<LightData>> uniformBuffer{nullptr};

public:
  VkPipelineBindPoint bindPoint;

  Pipeline(GraphicsDevice* device, Material* material);
  void bake(const std::shared_ptr<const RenderPass>&renderPass, uint32_t subpassIndex, std::span<VkDescriptorSetLayout> layouts, std::deque<std::tuple<void*, std::function<void(void*)>>>& miscMemoryPool, std::vector<VkGraphicsPipelineCreateInfo>&createInfos, std::vector<VkPipeline*>& pipelines);
  void writeDescriptorSets(std::deque<std::tuple<void*, std::function<void(void*)>>>& miscMemoryPool, std::vector<VkWriteDescriptorSet>& writes, const RenderGraph&graph) override;
  ~Pipeline() override;

  [[nodiscard]] VkPipeline getPipeline() const;
  [[nodiscard]] VkPipelineLayout getLayout() const;
  [[nodiscard]] Material* getMaterial() const;
};