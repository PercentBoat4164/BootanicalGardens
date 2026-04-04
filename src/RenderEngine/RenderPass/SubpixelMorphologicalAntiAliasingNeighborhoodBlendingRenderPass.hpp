#pragma once

#include "src/RenderEngine/RenderPass/RenderPass.hpp"

#include <glm/vec4.hpp>

template<typename T> class UniformBuffer;

class SubpixelMorphologicalAntiAliasingNeighborhoodBlendingRenderPass : public RenderPass {
  struct PassData { glm::vec4 resolution; };
  std::unique_ptr<UniformBuffer<PassData>> uniformBuffer;

public:
  explicit SubpixelMorphologicalAntiAliasingNeighborhoodBlendingRenderPass(RenderGraph& graph);

  void setup() override;
  void bake(const std::vector<VkAttachmentDescription>& attachmentDescriptions, const std::vector<const Image*>& images) override;
  void writeDescriptorSets(std::deque<std::tuple<void*, std::function<void(void*)>>>& miscMemoryPool, std::vector<VkWriteDescriptorSet>& writes) override;
  void update() override;
  void execute(CommandBuffer& commandBuffer) override;

  Material* material;
};
